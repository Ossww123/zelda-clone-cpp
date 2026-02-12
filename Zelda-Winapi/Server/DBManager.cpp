#include "pch.h"
#include "DBManager.h"
#include "sqlite3.h"

DBManager& DBManager::GetInstance()
{
	static DBManager instance;
	return instance;
}

bool DBManager::Init(const string& dbPath)
{
	int rc = sqlite3_open(dbPath.c_str(), &_db);
	if (rc != SQLITE_OK)
	{
		cout << "[DB] Failed to open database: " << sqlite3_errmsg(_db) << endl;
		return false;
	}

	// WAL 모드 활성화 (성능 향상)
	sqlite3_exec(_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

	CreateTables();

	cout << "[DB] Database initialized: " << dbPath << endl;
	return true;
}

void DBManager::Close()
{
	if (_db)
	{
		sqlite3_close(_db);
		_db = nullptr;
		cout << "[DB] Database closed" << endl;
	}
}

void DBManager::CreateTables()
{
	const char* sql = R"(
		CREATE TABLE IF NOT EXISTS accounts (
			account_id INTEGER PRIMARY KEY AUTOINCREMENT,
			username TEXT UNIQUE NOT NULL,
			created_at TEXT DEFAULT (datetime('now'))
		);

		CREATE TABLE IF NOT EXISTS players (
			account_id INTEGER PRIMARY KEY,
			name TEXT NOT NULL,
			level INTEGER DEFAULT 1,
			exp INTEGER DEFAULT 0,
			hp INTEGER DEFAULT 100,
			FOREIGN KEY (account_id) REFERENCES accounts(account_id)
		);

		CREATE TABLE IF NOT EXISTS inventory (
			account_id INTEGER NOT NULL,
			slot INTEGER NOT NULL,
			item_id INTEGER NOT NULL,
			count INTEGER DEFAULT 1,
			PRIMARY KEY (account_id, slot)
		);

		CREATE TABLE IF NOT EXISTS equipment (
			account_id INTEGER NOT NULL,
			equip_type INTEGER NOT NULL,
			item_id INTEGER NOT NULL,
			count INTEGER DEFAULT 1,
			PRIMARY KEY (account_id, equip_type)
		);
	)";

	char* errMsg = nullptr;
	int rc = sqlite3_exec(_db, sql, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK)
	{
		cout << "[DB] CreateTables error: " << errMsg << endl;
		sqlite3_free(errMsg);
	}
}

int64 DBManager::FindOrCreateAccount(const string& username)
{
	// SELECT 먼저
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "SELECT account_id FROM accounts WHERE username = ?;";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(stmt) == SQLITE_ROW)
			{
				int64 id = sqlite3_column_int64(stmt, 0);
				sqlite3_finalize(stmt);
				cout << "[DB] Account found: " << username << " (id=" << id << ")" << endl;
				return id;
			}
			sqlite3_finalize(stmt);
		}
	}

	// INSERT
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "INSERT INTO accounts (username) VALUES (?);";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(stmt) == SQLITE_DONE)
			{
				int64 id = sqlite3_last_insert_rowid(_db);
				sqlite3_finalize(stmt);
				cout << "[DB] Account created: " << username << " (id=" << id << ")" << endl;
				return id;
			}
			sqlite3_finalize(stmt);
		}
	}

	return 0;
}

bool DBManager::HasPlayerData(int64 accountId)
{
	sqlite3_stmt* stmt = nullptr;
	const char* sql = "SELECT 1 FROM players WHERE account_id = ?;";
	if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return false;

	sqlite3_bind_int64(stmt, 1, accountId);
	bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
	sqlite3_finalize(stmt);
	return exists;
}

bool DBManager::LoadPlayerData(int64 accountId, PlayerSaveData& outData)
{
	// players 테이블
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "SELECT name, level, exp, hp FROM players WHERE account_id = ?;";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
			return false;

		sqlite3_bind_int64(stmt, 1, accountId);
		if (sqlite3_step(stmt) != SQLITE_ROW)
		{
			sqlite3_finalize(stmt);
			return false;
		}

		outData.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		outData.level = sqlite3_column_int(stmt, 1);
		outData.exp = sqlite3_column_int(stmt, 2);
		outData.hp = sqlite3_column_int(stmt, 3);
		sqlite3_finalize(stmt);
	}

	// inventory 테이블
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "SELECT slot, item_id, count FROM inventory WHERE account_id = ?;";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int64(stmt, 1, accountId);
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				int32 slot = sqlite3_column_int(stmt, 0);
				if (slot >= 0 && slot < Player::INVENTORY_SIZE)
				{
					outData.storage[slot].itemId = sqlite3_column_int(stmt, 1);
					outData.storage[slot].count = sqlite3_column_int(stmt, 2);
				}
			}
			sqlite3_finalize(stmt);
		}
	}

	// equipment 테이블
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "SELECT equip_type, item_id, count FROM equipment WHERE account_id = ?;";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int64(stmt, 1, accountId);
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				int32 equipType = sqlite3_column_int(stmt, 0);
				int32 itemId = sqlite3_column_int(stmt, 1);
				int32 count = sqlite3_column_int(stmt, 2);

				if (equipType == 0)
				{
					outData.equipWeapon.itemId = itemId;
					outData.equipWeapon.count = count;
				}
				else if (equipType == 1)
				{
					outData.equipArmor.itemId = itemId;
					outData.equipArmor.count = count;
				}
				else if (equipType == 2)
				{
					outData.equipPotion.itemId = itemId;
					outData.equipPotion.count = count;
				}
			}
			sqlite3_finalize(stmt);
		}
	}

	cout << "[DB] Loaded player: " << outData.name << " (Lv." << outData.level << ")" << endl;
	return true;
}

bool DBManager::SavePlayerData(int64 accountId, const PlayerSaveData& data)
{
	sqlite3_exec(_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

	// players UPSERT
	{
		sqlite3_stmt* stmt = nullptr;
		const char* sql = "INSERT OR REPLACE INTO players (account_id, name, level, exp, hp) VALUES (?, ?, ?, ?, ?);";
		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int64(stmt, 1, accountId);
			sqlite3_bind_text(stmt, 2, data.name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 3, data.level);
			sqlite3_bind_int(stmt, 4, data.exp);
			sqlite3_bind_int(stmt, 5, data.hp);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// inventory: 기존 삭제 후 재삽입
	{
		sqlite3_stmt* delStmt = nullptr;
		const char* delSql = "DELETE FROM inventory WHERE account_id = ?;";
		if (sqlite3_prepare_v2(_db, delSql, -1, &delStmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int64(delStmt, 1, accountId);
			sqlite3_step(delStmt);
			sqlite3_finalize(delStmt);
		}

		sqlite3_stmt* insStmt = nullptr;
		const char* insSql = "INSERT INTO inventory (account_id, slot, item_id, count) VALUES (?, ?, ?, ?);";
		if (sqlite3_prepare_v2(_db, insSql, -1, &insStmt, nullptr) == SQLITE_OK)
		{
			for (int32 i = 0; i < Player::INVENTORY_SIZE; i++)
			{
				if (data.storage[i].itemId == 0)
					continue;

				sqlite3_bind_int64(insStmt, 1, accountId);
				sqlite3_bind_int(insStmt, 2, i);
				sqlite3_bind_int(insStmt, 3, data.storage[i].itemId);
				sqlite3_bind_int(insStmt, 4, data.storage[i].count);
				sqlite3_step(insStmt);
				sqlite3_reset(insStmt);
			}
			sqlite3_finalize(insStmt);
		}
	}

	// equipment: 기존 삭제 후 재삽입
	{
		sqlite3_stmt* delStmt = nullptr;
		const char* delSql = "DELETE FROM equipment WHERE account_id = ?;";
		if (sqlite3_prepare_v2(_db, delSql, -1, &delStmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int64(delStmt, 1, accountId);
			sqlite3_step(delStmt);
			sqlite3_finalize(delStmt);
		}

		const InventorySlot equips[] = { data.equipWeapon, data.equipArmor, data.equipPotion };
		sqlite3_stmt* insStmt = nullptr;
		const char* insSql = "INSERT INTO equipment (account_id, equip_type, item_id, count) VALUES (?, ?, ?, ?);";
		if (sqlite3_prepare_v2(_db, insSql, -1, &insStmt, nullptr) == SQLITE_OK)
		{
			for (int32 i = 0; i < 3; i++)
			{
				if (equips[i].itemId == 0)
					continue;

				sqlite3_bind_int64(insStmt, 1, accountId);
				sqlite3_bind_int(insStmt, 2, i);
				sqlite3_bind_int(insStmt, 3, equips[i].itemId);
				sqlite3_bind_int(insStmt, 4, equips[i].count);
				sqlite3_step(insStmt);
				sqlite3_reset(insStmt);
			}
			sqlite3_finalize(insStmt);
		}
	}

	sqlite3_exec(_db, "COMMIT;", nullptr, nullptr, nullptr);

	cout << "[DB] Saved player: " << data.name << " (Lv." << data.level << ", Exp=" << data.exp << ")" << endl;
	return true;
}
