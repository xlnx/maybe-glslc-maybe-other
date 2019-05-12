#include "expression.h"

static QualifiedValue handle_binary_expr( Json::Value &node, VoidType const &_ )
{
	auto &children = node[ "children" ];
	auto lhs = get<QualifiedValue>( codegen( children[ 0 ], _ ) ).value( children[ 0 ] );
	auto rhs = get<QualifiedValue>( codegen( children[ 2 ], _ ) ).value( children[ 2 ] );
	auto op = children[ 1 ][ 1 ].asCString();

	static JumpTable<QualifiedValue( QualifiedValue & lhs, QualifiedValue & rhs, Json::Value & ast )> __ = {
		{ "*", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( type->is<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateMul( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFMul( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "/", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateSDiv( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateUDiv( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFDiv( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "%", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateSRem( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateURem( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFRem( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "+", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 if ( lhs.get_type()->is<mty::Derefable>() && rhs.get_type()->is<mty::Integer>() )
			 {
				 return lhs.offset( rhs.get(), ast[ "children" ][ 0 ] );
			 }
			 else if ( lhs.get_type()->is<mty::Integer>() && rhs.get_type()->is<mty::Derefable>() )
			 {
				 return rhs.offset( lhs.get(), ast[ "children" ][ 2 ] );
			 }
			 else
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 auto &type = lhs.get_type();
				 if ( auto itype = type->as<mty::Integer>() )
				 {
					 return QualifiedValue( type, Builder.CreateAdd( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFAdd( lhs.get(), rhs.get() ) );
				 }
			 }
		 } },
		{ "-", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 TODO( "pointer arithmetic not implemented" );
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateSub( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFSub( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "<<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateShl( lhs.get(), rhs.get() ) );
		 } },
		{ ">>", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 auto type = lhs.get_type()->as<mty::Integer>();
			 if ( type->is_signed )
			 {
				 return QualifiedValue( lhs.get_type(), Builder.CreateAShr( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( lhs.get_type(), Builder.CreateLShr( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateICmpSLT( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateICmpULT( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOLT( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ ">", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateICmpSGT( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateICmpUGT( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOGT( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "<=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateICmpSLE( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateICmpULE( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOLE( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ ">=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateICmpSGE( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateICmpUGE( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOGE( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "==", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateICmpEQ( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOEQ( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "!=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateICmpNE( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpONE( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "&", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateAnd( lhs.get(), rhs.get() ) );
		 } },
		{ "^", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateXor( lhs.get(), rhs.get() ) );
		 } },
		{ "|", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateOr( lhs.get(), rhs.get() ) );
		 } },

		{ "&&", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } },
		{ "||", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } }
	};

	if ( __.find( op ) != __.end() )
	{
		return __[ op ]( lhs, rhs, node );
	}
	else
	{
		INTERNAL_ERROR( op );
	}
}

int Expression::reg()
{
	static decltype( handlers ) expr = {
		{ "assignment_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) );
			  auto type = lhs.get_type();

			  if ( type->is<mty::Address>() || type->is<mty::Void>() )
			  {
				  infoList->add_msg( MSG_TYPE_ERROR,
									 fmt( "object of type `", type, "` is not assignable" ),
									 children[ 0 ] );
				  HALT();
			  }
			  if ( type->is_const )
			  {
				  infoList->add_msg( MSG_TYPE_ERROR,
									 fmt( "cannot assign to const-qualified type `", type, "`" ),
									 children[ 0 ] );
				  HALT();
			  }
			  lhs = lhs.deref( children[ 0 ] );

			  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) )
						   .value( children[ 2 ] )
						   .cast_into_storage( type, children[ 2 ] );

			  return QualifiedValue( lhs.get_type(), Builder.CreateStore( rhs.get(), lhs.get() ) );
		  } ) },
		{ "unary_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  if ( children.size() == 2 )
			  {
				  auto key = children[ 0 ][ 1 ].asCString();
				  auto val = get<QualifiedValue>( codegen( children[ 1 ] ) );

				  static JumpTable<QualifiedValue( Json::Value & children, QualifiedValue & val, Json::Value & ast )> __ = {
					  { "*", []( Json::Value &children, QualifiedValue &val, Json::Value &ast ) -> QualifiedValue {
						   return val.value( children[ 1 ] ).deref( ast );
					   } }
				  };

				  if ( __.find( key ) != __.end() )
				  {
					  return __[ key ]( children, val, node );
				  }
				  else
				  {
					  UNIMPLEMENTED();
				  }
			  }
			  else
			  {
				  // sizeof ( TYPE )
				  UNIMPLEMENTED();
			  }
		  } ) },
		{ "postfix_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  auto key = children[ 1 ][ 1 ].asCString();
			  auto val = get<QualifiedValue>( codegen( children[ 0 ] ) );

			  static JumpTable<QualifiedValue( Json::Value & children, QualifiedValue & val, Json::Value & )> __ = {
				  { "[", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   auto off = get<QualifiedValue>( codegen( children[ 2 ] ) );
					   return val.value( children[ 0 ] ).offset( off.value( children[ 2 ] ).get(), node );
				   } },
				  { "++", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   UNIMPLEMENTED();
					   //    auto off = get<QualifiedValue>( codegen( children[ 2 ] ) );
					   //    return val.value( children[ 0 ] ).offset( off.value( children[ 2 ] ).get(), node );
				   } }
			  };

			  if ( __.find( key ) != __.end() )
			  {
				  return __[ key ]( children, val, node );
			  }
			  else
			  {
				  UNIMPLEMENTED();
			  }
		  } ) },
		{ "primary_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  if ( children.size() > 1 )
			  {  // ( expr )
				  return get<QualifiedValue>( codegen( children[ 1 ] ) );
			  }
			  else
			  {  // deal with Identifer | Literal
				  auto &child = children[ 0 ];
				  auto val = child[ 1 ].asCString();
				  auto type = child[ 0 ].asCString();

				  static JumpTable<QualifiedValue( const char *, Json::Value & )> __ = {
					  { "IDENTIFIER", []( const char *val, Json::Value &node ) -> QualifiedValue {
						   if ( auto sym = symTable.find( val ) )
						   {
							   if ( sym->is_value() )
							   {
								   return sym->as_value();
							   }
							   else
							   {
								   INTERNAL_ERROR();
							   }
						   }
						   else
						   {
							   infoList->add_msg( MSG_TYPE_ERROR, fmt( "use of undeclared identifier `", val, "`" ), node );
							   HALT();
						   }
					   } },
					  { "INTEGER", []( const char *val, Json::Value &node ) -> QualifiedValue {
						   auto num = std::strtoll( val, nullptr, 0 );
						   bool is_signed = true;
						   bool is_long = false;

						   while ( *val != 0 )
						   {
							   switch ( *val++ )
							   {
							   case 'u':
							   case 'U': is_signed = false; break;
							   case 'l':
							   case 'L': is_long = true; break;
							   }
						   }

						   auto &type = !is_long ? TypeView::getIntTy( is_signed ) : TypeView::getLongTy( is_signed );
						   return QualifiedValue( type, Constant::getIntegerValue(
														  type->type, APInt( type->as<mty::Integer>()->bits, num, is_signed ) ) );
					   } },
					  { "FLOATING_POINT", []( const char *val, Json::Value &node ) -> QualifiedValue {
						   auto num = std::strtold( val, nullptr );
						   int is_double = 0;

						   while ( *val != 0 )
						   {
							   switch ( *val++ )
							   {
							   case 'f':
							   case 'F': is_double = -1; break;
							   case 'l':
							   case 'L':
								   is_double = 1;
								   infoList->add_msg( MSG_TYPE_WARNING, "`long double` literal is not currently supported", node );
								   break;
							   }
						   }
						   auto &type = is_double == 0 ? TypeView::getDoubleTy() : is_double == 1 ? TypeView::getLongDoubleTy() : TypeView::getFloatTy();

						   return QualifiedValue( type, ConstantFP::get( type->type, num ) );
					   } }
				  };

				  if ( __.find( type ) != __.end() )
				  {
					  return __[ type ]( val, child );
				  }
				  else
				  {
					  UNIMPLEMENTED();
					  //   INTERNAL_ERROR();
				  }
			  }
		  } ) },

		{ "binary_expression", pack_fn<VoidType, QualifiedValue>( handle_binary_expr ) }
	};

	handlers.insert( expr.begin(), expr.end() );

	return 0;
}