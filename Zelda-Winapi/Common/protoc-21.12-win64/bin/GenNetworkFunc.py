import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# ----- markers -----
AUTO_ENUM_BEGIN = r"// \[AUTO-GEN ENUM BEGIN\]"
AUTO_ENUM_END   = r"// \[AUTO-GEN ENUM END\]"

AUTO_DECL_BEGIN = r"// \[AUTO-GEN DECLS BEGIN\]"
AUTO_DECL_END   = r"// \[AUTO-GEN DECLS END\]"

AUTO_SWITCH_BEGIN = r"// \[AUTO-GEN SWITCH BEGIN\]"
AUTO_SWITCH_END   = r"// \[AUTO-GEN SWITCH END\]"

# ----- proto marker -----
NEW_PACKET_MARKER = "[NEW PACKET]"

# ----- regex -----
MESSAGE_LINE_RE = re.compile(r"^\s*message\s+([A-Za-z_]\w*)\s*(?:\{|$)")
ENUM_ENTRY_RE = re.compile(r"^\s*([A-Za-z_]\w*)\s*=\s*(\d+)\s*,?\s*$", re.MULTILINE)
CASE_RE = re.compile(r"\bcase\s+([A-Za-z_]\w*)\s*:")

# function-name extractor (whole header)
DECL_FN_RE = re.compile(r"\b(?:static\s+)?(?:void|SendBufferRef)\s+(Handle_[A-Za-z_]\w*|Make_[A-Za-z_]\w*)\s*\(")

# proto field line (simple)
# ex) repeated BuffData buffs = 4;
#     int32 testValueONE = 1;
#     DIR_TYPE dir = 1;
FIELD_LINE_RE = re.compile(
    r"^\s*(repeated\s+)?([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*=\s*\d+\s*(?:\[[^\]]*\])?\s*;\s*$"
)

# ------------------------------------------------------------
# file io
# ------------------------------------------------------------
def read_text(p: Path) -> str:
    return p.read_text(encoding="utf-8", errors="ignore")

def write_text(p: Path, s: str) -> None:
    p.write_text(s, encoding="utf-8")

# ------------------------------------------------------------
# marker block helpers
# ------------------------------------------------------------
def find_block_range(text: str, begin_pat: str, end_pat: str) -> Tuple[int, int, int, int]:
    b = re.search(begin_pat, text)
    e = re.search(end_pat, text)
    if not b or not e or b.end() > e.start():
        raise RuntimeError(f"Markers not found or invalid: {begin_pat} ~ {end_pat}")
    return b.start(), b.end(), e.start(), e.end()

def get_block(text: str, begin_pat: str, end_pat: str) -> str:
    _, b_end, e_start, _ = find_block_range(text, begin_pat, end_pat)
    return text[b_end:e_start]

def replace_block(text: str, begin_pat: str, end_pat: str, new_block: str) -> str:
    b = re.search(begin_pat, text)
    e = re.search(end_pat, text)
    if not b or not e or b.end() > e.start():
        raise RuntimeError(f"Markers not found or invalid: {begin_pat} ~ {end_pat}")

    # BEGIN 마커 라인의 indent를 읽어서 END 마커 라인에도 적용
    begin_line_start = text.rfind("\n", 0, b.start()) + 1
    begin_indent = re.match(r"[ \t]*", text[begin_line_start:b.start()]).group(0)

    # END 마커 라인이 있는 줄의 시작~끝 범위를 찾음
    end_line_start = text.rfind("\n", 0, e.start()) + 1
    end_line_end = text.find("\n", e.start())
    if end_line_end == -1:
        end_line_end = len(text)

    end_line = text[end_line_start:end_line_end]
    end_line_no_indent = end_line.lstrip(" \t")

    # END 마커 라인을 BEGIN indent로 교정
    fixed_end_line = begin_indent + end_line_no_indent

    # 본문 치환 + END 라인 교정 반영
    before = text[:b.end()] + "\n" + new_block.rstrip() + "\n" + text[b.end():e.start()]
    after = fixed_end_line + text[end_line_end:]
    return text[:b.end()] + "\n" + new_block.rstrip() + "\n" + text[b.end():end_line_start] + fixed_end_line + text[end_line_end:]


def detect_indent_from_block(block: str, default: str = "\t") -> str:
    for line in block.splitlines():
        if line.strip():
            return re.match(r"^\s*", line).group(0)
    return default

# ------------------------------------------------------------
# proto parsing: only [NEW PACKET] messages + their fields
# ------------------------------------------------------------
def parse_new_packet_messages(proto_text: str, marker: str = NEW_PACKET_MARKER) -> List[str]:
    out: List[str] = []
    want_next = False
    for line in proto_text.splitlines():
        if marker in line:
            want_next = True
            continue

        m = MESSAGE_LINE_RE.match(line)
        if m and want_next:
            out.append(m.group(1))
            want_next = False

    return [m for m in out if m.startswith("C_") or m.startswith("S_")]

def extract_message_block(proto_text: str, message_name: str) -> Optional[str]:
    """
    message <name> { ... } 블록 문자열 반환(브레이스 카운트).
    """
    lines = proto_text.splitlines()
    start_idx = None
    for i, line in enumerate(lines):
        m = MESSAGE_LINE_RE.match(line)
        if m and m.group(1) == message_name:
            start_idx = i
            break
    if start_idx is None:
        return None

    # brace counting from the first '{' on start line or subsequent lines
    depth = 0
    started = False
    buf: List[str] = []

    for j in range(start_idx, len(lines)):
        line = lines[j]
        buf.append(line)

        # count braces char-by-char
        for ch in line:
            if ch == "{":
                depth += 1
                started = True
            elif ch == "}":
                depth -= 1

        if started and depth == 0:
            break

    return "\n".join(buf)

def parse_fields_from_message_block(block: str) -> List[Tuple[bool, str, str]]:
    """
    return list of (is_repeated, proto_type, field_name) in order.
    """
    fields: List[Tuple[bool, str, str]] = []
    for line in block.splitlines():
        m = FIELD_LINE_RE.match(line)
        if not m:
            continue
        is_rep = bool(m.group(1))
        f_type = m.group(2)
        f_name = m.group(3)
        fields.append((is_rep, f_type, f_name))
    return fields

def build_fields_map(proto_text: str, msgs: List[str]) -> Dict[str, List[Tuple[bool, str, str]]]:
    out: Dict[str, List[Tuple[bool, str, str]]] = {}
    for msg in msgs:
        block = extract_message_block(proto_text, msg)
        if not block:
            out[msg] = []
            continue
        out[msg] = parse_fields_from_message_block(block)
    return out

# ------------------------------------------------------------
# enum / id management
# ------------------------------------------------------------
def extract_enum_ids_anywhere(text: str) -> Dict[str, int]:
    ids: Dict[str, int] = {}
    for m in ENUM_ENTRY_RE.finditer(text):
        ids[m.group(1)] = int(m.group(2))
    return ids

def next_id_for_prefix(id_map: Dict[str, int], prefix: str, start: int) -> int:
    vals = [v for k, v in id_map.items() if k.startswith(prefix)]
    if not vals:
        return start
    return max(max(vals) + 1, start)

def assign_ids_for_new_messages(
    existing_ids: Dict[str, int],
    new_msgs: List[str],
    c_start: int = 101,
    s_start: int = 201,
) -> Dict[str, int]:
    out = dict(existing_ids)
    used = set(out.values())

    c_next = next_id_for_prefix(out, "C_", c_start)
    s_next = next_id_for_prefix(out, "S_", s_start)

    for name in new_msgs:
        if name in out:
            continue

        if name.startswith("C_"):
            nid = c_next
            while nid in used:
                nid += 1
            out[name] = nid
            used.add(nid)
            c_next = nid + 1

        elif name.startswith("S_"):
            nid = s_next
            while nid in used:
                nid += 1
            out[name] = nid
            used.add(nid)
            s_next = nid + 1

    return out

def merge_enum_block(existing_block: str, id_map: Dict[str, int], new_msgs: List[str]) -> str:
    block_ids = extract_enum_ids_anywhere(existing_block)

    for n in new_msgs:
        if n not in block_ids and n in id_map:
            block_ids[n] = id_map[n]

    ordered = sorted(block_ids.keys(), key=lambda x: (0 if x.startswith("C_") else 1, x))

    indent = detect_indent_from_block(existing_block, "\t")  # ✅ 추가

    lines: List[str] = []
    last_prefix = None
    for name in ordered:
        prefix = "C_" if name.startswith("C_") else ("S_" if name.startswith("S_") else "")
        if last_prefix is not None and prefix != last_prefix:
            lines.append("")
        lines.append(f"{indent}{name} = {block_ids[name]} ,")  # ✅ indent 사용
        last_prefix = prefix

    return "\n".join(lines).rstrip()

# ------------------------------------------------------------
# decls merge (Handle + Make) with field-based params
# ------------------------------------------------------------
PROTO_SCALARS = {
    "double": "double",
    "float": "float",
    "int32": "int32",
    "int64": "int64",
    "uint32": "uint32",
    "uint64": "uint64",
    "sint32": "int32",
    "sint64": "int64",
    "fixed32": "uint32",
    "fixed64": "uint64",
    "sfixed32": "int32",
    "sfixed64": "int64",
    "bool": "bool",
    "string": "string",  # handled specially (const string&)
    "bytes": "string",   # handled specially (const string&)
}

def cpp_type_for_proto(proto_type: str) -> str:
    """
    - scalar -> int32/uint64/... (네 프로젝트 typedef 가정)
    - string/bytes -> string (-> const string&)
    - otherwise -> Protocol::<Type>
    """
    if proto_type in PROTO_SCALARS:
        return PROTO_SCALARS[proto_type]
    # enum/message 둘 다 Protocol 네임스페이스로
    return f"Protocol::{proto_type}"

def cpp_param_for_field(is_repeated: bool, proto_type: str, field_name: str) -> str:
    """
    파라미터 타입/전달 방식:
    - scalar/enum: by value
    - string/bytes: const string&
    - message: const Protocol::Type&
    - repeated: const vector<T>&
    """
    base_cpp = cpp_type_for_proto(proto_type)

    # string/bytes
    if proto_type in ("string", "bytes"):
        base_cpp = "string"

    # repeated
    if is_repeated:
        # vector<...> 형태
        # message/enum도 base_cpp가 Protocol::Type로 들어가므로 OK
        return f"const vector<{base_cpp}>& {field_name}"

    # non-repeated
    if proto_type in PROTO_SCALARS:
        # string/bytes는 const ref
        if proto_type in ("string", "bytes"):
            return f"const {base_cpp}& {field_name}"
        return f"{base_cpp} {field_name}"

    # non-scalar (enum/message) -> enum은 값으로, message는 const ref가 더 좋음
    # enum과 message를 구분하기 어렵기 때문에, 안전하게 const ref를 사용(복사 비용 방지)
    return f"const {base_cpp}& {field_name}"

def make_param_list(fields: List[Tuple[bool, str, str]]) -> str:
    if not fields:
        return ""
    parts = [cpp_param_for_field(rep, t, n) for (rep, t, n) in fields]
    return " , ".join(parts)

def extract_declared_functions_anywhere(header_text: str) -> set[str]:
    return set(DECL_FN_RE.findall(header_text))

def gen_client_handle_decl(msg: str) -> str:
    return f"\tstatic void Handle_{msg} ( ServerSessionRef session , BYTE* buffer , int32 len );"

def gen_server_handle_decl(msg: str) -> str:
    return f"\tstatic void Handle_{msg} ( GameSessionRef session , BYTE* buffer , int32 len );"

def gen_client_make_decl_field_based(msg: str, fields: List[Tuple[bool, str, str]]) -> str:
    params = make_param_list(fields)
    return f"\tstatic SendBufferRef Make_{msg} ( {params} );" if params else f"\tstatic SendBufferRef Make_{msg} ( );"

def gen_server_make_decl_field_based(msg: str, fields: List[Tuple[bool, str, str]]) -> str:
    params = make_param_list(fields)
    return f"\tstatic SendBufferRef Make_{msg} ( {params} );" if params else f"\tstatic SendBufferRef Make_{msg} ( );"

def merge_decls_block(
    existing_block: str,
    full_header_text: str,
    new_msgs: List[str],
    fields_map: Dict[str, List[Tuple[bool, str, str]]],
    side: str,
) -> str:
    """
    side:
      - client: Handle_S_*, Make_C_* (field-based)
      - server: Handle_C_*, Make_S_* (field-based)
    """
    already = extract_declared_functions_anywhere(full_header_text)

    lines = existing_block.rstrip("\n").splitlines()
    to_add: List[str] = []

    if side == "client":
        # Handle_S_*
        for m in [x for x in new_msgs if x.startswith("S_")]:
            fn = f"Handle_{m}"
            if fn not in already:
                to_add.append(gen_client_handle_decl(m))
                already.add(fn)

        # Make_C_* (field-based)
        for m in [x for x in new_msgs if x.startswith("C_")]:
            fn = f"Make_{m}"
            if fn not in already:
                to_add.append(gen_client_make_decl_field_based(m, fields_map.get(m, [])))
                already.add(fn)

    else:
        # Handle_C_*
        for m in [x for x in new_msgs if x.startswith("C_")]:
            fn = f"Handle_{m}"
            if fn not in already:
                to_add.append(gen_server_handle_decl(m))
                already.add(fn)

        # Make_S_* (field-based)
        for m in [x for x in new_msgs if x.startswith("S_")]:
            fn = f"Make_{m}"
            if fn not in already:
                to_add.append(gen_server_make_decl_field_based(m, fields_map.get(m, [])))
                already.add(fn)

    if not to_add:
        return existing_block.rstrip()

    if existing_block.strip():
        lines.append("")
    lines.extend(to_add)
    return "\n".join(lines).rstrip()

# ------------------------------------------------------------
# switch merge (case add only)
# ------------------------------------------------------------
def extract_cases(block: str) -> set[str]:
    return set(CASE_RE.findall(block))

def gen_case_lines(msg: str, indent_case: str) -> List[str]:
    indent_inner = indent_case + "\t"
    return [
        f"{indent_case}case {msg}:",
        f"{indent_inner}Handle_{msg} ( session , buffer , len );",
        f"{indent_inner}break;",
    ]

def merge_switch_block(existing_block: str, new_msgs: List[str], side: str) -> str:
    existing_cases = extract_cases(existing_block)
    indent = detect_indent_from_block(existing_block, "\t")

    targets = [m for m in new_msgs if (m.startswith("S_") if side == "client" else m.startswith("C_"))]

    add_lines: List[str] = []
    for m in targets:
        if m in existing_cases:
            continue
        if add_lines:
            add_lines.append("")
        add_lines.extend(gen_case_lines(m, indent))

    if not add_lines:
        return existing_block.rstrip()

    out = existing_block.rstrip("\n")
    if out.strip():
        out += "\n"
    out += "\n".join(add_lines).rstrip()
    return out.rstrip()

# ------------------------------------------------------------
# main
# ------------------------------------------------------------
def main():
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parents[2]  # Zelda-Winapi

    proto_path = script_dir / "Protocol.proto"
    if not proto_path.exists():
        raise FileNotFoundError(f"Protocol.proto not found next to script: {proto_path}")

    client_h   = project_root / "Client" / "ClientPacketHandler.h"
    client_cpp = project_root / "Client" / "ClientPacketHandler.cpp"
    server_h   = project_root / "Server" / "ServerPacketHandler.h"
    server_cpp = project_root / "Server" / "ServerPacketHandler.cpp"


    for p in [client_h, client_cpp, server_h, server_cpp]:
        if not p.exists():
            raise FileNotFoundError(f"File not found: {p}")

    proto_text = read_text(proto_path)
    new_msgs = parse_new_packet_messages(proto_text, NEW_PACKET_MARKER)

    if not new_msgs:
        print(f"[INFO] No messages marked with // {NEW_PACKET_MARKER}")
        return

    # new_msgs의 필드 파싱
    fields_map = build_fields_map(proto_text, new_msgs)

    client_h_text = read_text(client_h)
    server_h_text = read_text(server_h)

    # 기존 id 수집(양쪽 헤더에서)
    existing_ids = extract_enum_ids_anywhere(client_h_text)
    existing_ids.update(extract_enum_ids_anywhere(server_h_text))

    # 새 메시지에 id 부여(기존 유지)
    id_map = assign_ids_for_new_messages(existing_ids, new_msgs, c_start=101, s_start=201)

    # ---- client.h: enum block ----
    ch = client_h_text
    old_enum_block = get_block(ch, AUTO_ENUM_BEGIN, AUTO_ENUM_END)
    new_enum_block = merge_enum_block(old_enum_block, id_map, new_msgs)
    ch = replace_block(ch, AUTO_ENUM_BEGIN, AUTO_ENUM_END, new_enum_block)

    # ---- client.h: decl block (Handle_S + Make_C field-based) ----
    old_decl_block = get_block(ch, AUTO_DECL_BEGIN, AUTO_DECL_END)
    new_decl_block = merge_decls_block(old_decl_block, ch, new_msgs, fields_map, side="client")
    ch = replace_block(ch, AUTO_DECL_BEGIN, AUTO_DECL_END, new_decl_block)
    write_text(client_h, ch)

    # ---- client.cpp: switch block (S_ cases) ----
    cc = read_text(client_cpp)
    old_sw_block = get_block(cc, AUTO_SWITCH_BEGIN, AUTO_SWITCH_END)
    new_sw_block = merge_switch_block(old_sw_block, new_msgs, side="client")
    cc = replace_block(cc, AUTO_SWITCH_BEGIN, AUTO_SWITCH_END, new_sw_block)
    write_text(client_cpp, cc)

    # ---- server.h: enum block ----
    sh = server_h_text
    old_enum_block_s = get_block(sh, AUTO_ENUM_BEGIN, AUTO_ENUM_END)
    new_enum_block_s = merge_enum_block(old_enum_block_s, id_map, new_msgs)
    sh = replace_block(sh, AUTO_ENUM_BEGIN, AUTO_ENUM_END, new_enum_block_s)

    # ---- server.h: decl block (Handle_C + Make_S field-based) ----
    old_decl_block_s = get_block(sh, AUTO_DECL_BEGIN, AUTO_DECL_END)
    new_decl_block_s = merge_decls_block(old_decl_block_s, sh, new_msgs, fields_map, side="server")
    sh = replace_block(sh, AUTO_DECL_BEGIN, AUTO_DECL_END, new_decl_block_s)
    write_text(server_h, sh)

    # ---- server.cpp: switch block (C_ cases) ----
    sc = read_text(server_cpp)
    old_sw_block_s = get_block(sc, AUTO_SWITCH_BEGIN, AUTO_SWITCH_END)
    new_sw_block_s = merge_switch_block(old_sw_block_s, new_msgs, side="server")
    sc = replace_block(sc, AUTO_SWITCH_BEGIN, AUTO_SWITCH_END, new_sw_block_s)
    write_text(server_cpp, sc)

    # ---- Protocol.proto: remove // [NEW PACKET] markers after successful generation ----
    # marker 문자열이 포함된 라인 자체를 제거
    proto_lines = proto_text.splitlines()
    filtered = [ln for ln in proto_lines if NEW_PACKET_MARKER not in ln]

    # 줄바꿈 유지(대충이라도 원본 스타일 유지)
    new_proto_text = "\n".join(filtered).rstrip() + "\n"
    write_text(proto_path, new_proto_text)


    print("[OK] Updated enum/decls/switch blocks (merge). Implementations untouched.")
    print(f"[INFO] Proto: {proto_path}")
    print(f"[INFO] New messages: {new_msgs}")
    for m in new_msgs:
        print(f"  - {m} = {id_map.get(m)}")
        f = fields_map.get(m, [])
        if f:
            print(f"    params: {make_param_list(f)}")
        else:
            print("    params: (none)")

if __name__ == "__main__":
    main()
