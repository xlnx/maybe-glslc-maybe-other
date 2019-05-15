#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Struct : Structural
{
	static constexpr auto self_type = TypeName::StructType;

	mutable std::map<std::string, std::pair<QualifiedType, ConstantInt *>> comps;
	Option<std::string> name;

	Struct() :
	  Structural( StructType::create( TheContext ) )
	{
		type_name = self_type;
	}

	Struct( const std::string &name ) :
	  Structural( StructType::create( TheContext, "struct." + name ) )
	{
		type_name = self_type;
		this->name = name;
	}

	void set_body( const std::vector<QualifiedDecl> &comps, Json::Value &ast )
	{
		if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		{
			infoList->add_msg( MSG_TYPE_ERROR, fmt( "`struct ", name.unwrap(), "` redefined" ), ast );
			HALT();
		}
		static_cast<llvm::StructType *>( this->type )->setBody( map_comp( comps ) );
		unsigned index = 0;
		for ( auto &comp : comps )
		{
			if ( comp.name.is_some() )
			{
				auto ind = static_cast<ConstantInt *>(
				  Constant::getIntegerValue(
					TypeView::getIntTy( false )->type,
					APInt( 32, uint64_t(index), false ) ) );
				this->comps.emplace(
				  comp.name.unwrap(),
				  std::make_pair( comp.type, ind ) );
			}
			else
			{
				infoList->add_msg( MSG_TYPE_WARNING, "declaration does not declare anything" );
			}
		}
	}

	const std::pair<QualifiedType, ConstantInt *> &get_member( const std::string &member, Json::Value &ast ) const
	{
		if ( this->comps.find( member ) != this->comps.end() )
		{
			return this->comps.find( member )->second;
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "struct has no member named `", member, "`" ),
			  ast );
			HALT();
		}
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "struct";
		if ( this->name.is_some() ) os << " " << this->name.unwrap();
		// if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		// {
		// 	auto indent = decl_indent;
		// 	decl_indent += "  ";
		// 	os << " {\n";
		// 	std::vector<std::pair<const QualifiedType *, std::string>> __( this->comps.size() );
		// 	for ( auto &arg : this->comps )
		// 	{
		// 		__[ arg.second.second->getZExtValue() ] = std::make_pair( &arg.second.first, arg.first );
		// 	}
		// 	for ( auto &arg : __ )
		// 	{
		// 		os << decl_indent << arg.second << " : " << *arg.first << ";\n";
		// 	}
		// 	decl_indent = indent;
		// 	os << decl_indent << "}";
		// }
		if ( st.size() != ++id )
		{
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Struct>( *this );
	}

public:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<Struct const &>( other );
		return this->type == ref.type;
		// return this->name.is_some() && ref.name.is_some() &&
		// 	   this->name.unwrap() == ref.name.unwrap();
	}

private:
	static std::vector<Type *> map_comp( const std::vector<QualifiedDecl> &comps )
	{
		std::vector<Type *> new_args;
		for ( auto arg : comps ) new_args.push_back( arg.type->type );
		return new_args;
	}
};

}  // namespace mty
