#pragma once

#include "predef.h"

namespace mty
{
struct Void : Qualified
{
	static constexpr auto self_type = TypeName::VoidType;

	Void( bool is_const = false, bool is_volatile = false ) :
	  Qualified( Type::getVoidTy( TheContext ), is_const, is_volatile )
	{
		type_name = self_type;
	}

	bool is_valid_element_type() const override
	{
		return false;
	}

	bool is_valid_parameter_type() const override
	{
		return false;
	}

	bool is_allocable() const override
	{
		return false;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "void";
		if ( st.size() != ++id )
		{
			os << " ";
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Void>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		return true;
	}
};

}  // namespace mty
