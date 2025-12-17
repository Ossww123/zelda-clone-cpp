#pragma once

class Object;

class ObjectManager
{
	DECLARE_SINGLE ( ObjectManager );

public:
	~ObjectManager ( );

	void Add ( Object* object );
	void Remove ( Object* object );
	void Clear ( );

	const vector<Object*>& GetObjects ( ) { return _objects; }

	template<typename T>
	T* CreateObject ( )
	{
		// type trait
		static_assert( std::is_convertible_v<T* , Object*> );

		T* object = new T ( );
		object->Init ( );

		return object;
	}

private:
	vector<Object*> _objects;
};

