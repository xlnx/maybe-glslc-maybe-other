#pragma once

#include "common.h"

struct Qualified
{
	Type *type;
	bool is_const;
	bool is_volatile;

	Qualified( Type *type, bool is_const = false, bool is_volatile = false ) :
	  type( type ),
	  is_const( is_const ),
	  is_volatile( is_volatile )
	{
	}

	virtual ~Qualified() = default;
};

class QualifiedTypeBuilder;

class QualifiedType
{
	friend class QualifiedTypeBuilder;

private:
	std::vector<std::shared_ptr<Qualified>> list;

	QualifiedType() = default;

public:
	QualifiedType( Type *type )
	{
		list.emplace_back( std::make_shared<Qualified>( type ) );
	}

	operator Type *() const
	{
		return list.back()->type;
	}

	Type *operator->() const
	{
		return this->list.back()->type;
	}

	template <typename T>
	T *as() const
	{
		return static_cast<T *>( (Type *)*this );
	}

	friend std::ostream &operator<<( std::ostream &os, const QualifiedType &type );
};

struct QualifiedDecl
{
	QualifiedType type;
	Option<std::string> name;

public:
	QualifiedDecl( const QualifiedType &type, const std::string &name ) :
	  type( type ),
	  name( name )
	{
	}
	QualifiedDecl( const QualifiedType &type ) :
	  type( type )
	{
	}

	friend std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl );
};

struct QualifiedPointer : Qualified
{
	QualifiedPointer( Type *base_type, bool is_const = false, bool is_volatile = false ) :
	  Qualified( PointerType::getUnqual( base_type ), is_const, is_volatile )
	{
	}
};

struct QualifiedType;

struct QualifiedFunction : Qualified
{
	std::vector<QualifiedDecl> args;

	QualifiedFunction( Type *result_type, const std::vector<QualifiedDecl> &args ) :
	  Qualified( FunctionType::get( result_type, map_type( args ), false ) ),
	  args( args )
	{
	}

private:
	static std::vector<Type *> map_type( const std::vector<QualifiedDecl> &args )
	{
		std::vector<Type *> new_args;
		for ( auto arg : args ) new_args.push_back( arg.type );
		return new_args;
	}
};

struct QualifiedStruct : Qualified
{
	std::map<std::string, QualifiedType> comps;

	QualifiedStruct( const std::vector<QualifiedDecl> &comps ) :
	  Qualified( StructType::get( TheContext, map_comp( comps ), false ) )
	{
		for ( auto &comp : comps )
		{
			if ( comp.name.is_some() )
			{
				this->comps.emplace( comp.name.unwrap(), comp.type );
			}
			else
			{
				infoList->add_msg( MSG_TYPE_WARNING, "declaration does not declare anything" );
			}
		}
	}

private:
	static std::vector<Type *> map_comp( const std::vector<QualifiedDecl> &comps )
	{
		std::vector<Type *> new_args;
		for ( auto arg : comps ) new_args.push_back( arg.type );
		return new_args;
	}
};

class QualifiedTypeBuilder
{
private:
	std::vector<std::shared_ptr<Qualified>> list;

public:
	void add_level( const std::shared_ptr<Qualified> &obj )
	{
		list.emplace_back( obj );
	}
	Type *get_type() const
	{
		return list.back()->type;
	}
	QualifiedType build()
	{
		QualifiedType type;
		type.list = std::move( list );
		return type;
	}
};

std::ostream &operator<<( std::ostream &os, const QualifiedType &type )
{
	for ( int i = type.list.size() - 1; i >= 0; --i )
	{
		auto ptr = type.list[ i ].get();
		if ( auto fn = dynamic_cast<QualifiedFunction *>( ptr ) )
		{
			os << "Fn (";
			for ( auto arg : fn->args )
			{
				os << arg << ", ";
			}
			os << "): ";
		}
		else if ( dynamic_cast<QualifiedPointer *>( ptr ) )
		{
			os << "Ptr: ";
		}
		else if ( dynamic_cast<Qualified *>( ptr ) )
		{
			os << "Ty";
		}
		else
		{
			INTERNAL_ERROR();
		}
	}
	return os;
}

std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl )
{
	os << decl.name << " = " << decl.type;
	return os;
}