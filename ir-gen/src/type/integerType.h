#pragma once

#include "qualified.h"

struct QualifiedInteger : Qualified
{
	unsigned bits;
	bool is_signed;

	QualifiedInteger( unsigned bits, bool is_signed, bool is_const = false, bool is_volatile = false ) :
	  Qualified( Type::getIntNTy( TheContext, bits ), is_const, is_volatile ),
	  bits( bits ),
	  is_signed( is_signed )
	{
	}

	void print( std::ostream &os ) const override
	{
		Qualified::print( os );
		if ( is_signed )
			os << "i" << bits;
		else
			os << "u" << bits;
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<QualifiedInteger>( *this );
	}
};
