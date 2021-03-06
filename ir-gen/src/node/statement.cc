#include "statement.h"

int Statement::reg()
{
	static decltype( handlers ) stmt = {
		{ "compound_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];

			  symTable.push();

			  // Ignore the { and }
			  for ( int i = 1; i < children.size() - 1; i++ )
			  {
				  codegen( children[ i ] );
			  }

			  //   dbg( symTable );
			  symTable.pop();

			  return VoidType();
		  } ) },
		{ "expression_statement", pack_fn<VoidType, Option<QualifiedValue>>( []( Json::Value &node, VoidType const & ) -> Option<QualifiedValue> {
			  auto &children = node[ "children" ];

			  if ( children.size() > 1 )  // expr ;
			  {
				  return get<QualifiedValue>( codegen( children[ 0 ] ) );
			  }

			  return Option<QualifiedValue>();
		  } ) },
		{ "iteration_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto typeIter = children[ 0 ][ 1 ].asCString();
			  static JumpTable<VoidType( Json::Value & children, Json::Value & ast )> __ = {
				  { "while", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto func = currentFunction->get();
					   auto loopEnd = BasicBlock::Create( TheContext, "while.end", static_cast<Function *>( func ) );
					   auto loopBody = BasicBlock::Create( TheContext, "while.body", static_cast<Function *>( func ), loopEnd );
					   auto loopCond = BasicBlock::Create( TheContext, "while.cond", static_cast<Function *>( func ), loopBody );

					   Builder.CreateBr( loopCond );

					   Builder.SetInsertPoint( loopCond );
					   auto br = get<QualifiedValue>( codegen( children[ 2 ] ) )
								   .value( children[ 2 ] )
								   .cast( TypeView::getBoolTy(), children[ 2 ] );

					   Builder.CreateCondBr( br.get(), loopBody, loopEnd );

					   breakJump.emplace( loopEnd );
					   continueJump.emplace( loopCond );

					   Builder.SetInsertPoint( loopBody );
					   codegen( children[ 4 ] );

					   Builder.CreateBr( loopCond );

					   continueJump.pop();
					   breakJump.pop();

					   Builder.SetInsertPoint( loopEnd );

					   return VoidType();
				   } },
				  { "do", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto func = currentFunction->get();
					   auto loopEnd = BasicBlock::Create( TheContext, "do.end", static_cast<Function *>( func ) );
					   auto loopBody = BasicBlock::Create( TheContext, "do.body", static_cast<Function *>( func ), loopEnd );
					   auto loopCond = BasicBlock::Create( TheContext, "do.cond", static_cast<Function *>( func ), loopBody );

					   Builder.CreateBr( loopCond );

					   breakJump.emplace( loopEnd );
					   continueJump.emplace( loopCond );

					   Builder.SetInsertPoint( loopBody );
					   codegen( children[ 1 ] );
					   Builder.CreateBr( loopCond );

					   continueJump.pop();
					   breakJump.pop();

					   Builder.SetInsertPoint( loopCond );
					   auto br = get<QualifiedValue>( codegen( children[ 4 ] ) )
								   .value( children[ 2 ] )
								   .cast( TypeView::getBoolTy(), children[ 4 ] );
					   Builder.CreateCondBr( br.get(), loopBody, loopEnd );

					   Builder.SetInsertPoint( loopEnd );

					   return VoidType();
				   } },
				  { "for", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto func = currentFunction->get();
					   auto loopEnd = BasicBlock::Create( TheContext, "for.end", static_cast<Function *>( func ) );
					   auto loopInc = BasicBlock::Create( TheContext, "for.inc", static_cast<Function *>( func ), loopEnd );
					   auto loopBody = BasicBlock::Create( TheContext, "for.body", static_cast<Function *>( func ), loopInc );
					   auto loopCond = BasicBlock::Create( TheContext, "for.cond", static_cast<Function *>( func ), loopBody );

					   codegen( children[ 2 ] );
					   Builder.CreateBr( loopCond );

					   Builder.SetInsertPoint( loopCond );
					   auto br = get<Option<QualifiedValue>>( codegen( children[ 3 ] ) );
					   if ( br.is_some() )
					   {
						   auto br_val = br.unwrap()
										   .value( children[ 3 ] )
										   .cast( TypeView::getBoolTy(), children[ 3 ] );

						   Builder.CreateCondBr( br_val.get(), loopBody, loopEnd );
					   }
					   else
					   {
						   Builder.CreateBr( loopBody );
					   }

					   Builder.SetInsertPoint( loopInc );
					   if ( children[ 4 ].isObject() )
					   {
						   codegen( children[ 4 ] );
						   Builder.CreateBr( loopCond );
					   }
					   else
					   {
						   Builder.CreateBr( loopEnd );
					   }

					   breakJump.emplace( loopEnd );
					   continueJump.emplace( loopInc );

					   Builder.SetInsertPoint( loopBody );
					   int index = children[ 4 ].isObject() ? 6 : 5;
					   codegen( children[ index ] );
					   Builder.CreateBr( loopInc );

					   continueJump.pop();
					   breakJump.pop();

					   Builder.SetInsertPoint( loopEnd );

					   return VoidType();
				   } }
			  };
			  if ( __.find( typeIter ) != __.end() )
			  {
				  return __[ typeIter ]( children, node );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		{ "jump_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto typeRet = children[ 0 ][ 1 ].asCString();

			  static JumpTable<VoidType( Json::Value & children, Json::Value & ast )> __ = {
				  { "return", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto fn_res_ty = currentFunction->get_type();
					   fn_res_ty.next();
					   if ( children.size() == 3 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "void function `", funcName, "` should not return a value" ),
								 children[ 1 ] );
							   HALT();
						   }
						   auto retValue = get<QualifiedValue>( codegen( children[ 1 ] ) )
											 .value( children[ 1 ] )
											 .cast( fn_res_ty, children[ 1 ] );
						   Builder.CreateRet( retValue.get() );
					   }
					   else if ( children.size() == 2 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   Builder.CreateRet( nullptr );
						   }
						   else
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "non-void function `", funcName, "` should return a value" ),
								 ast );
							   HALT();
						   }
					   }
					   else
					   {
						   INTERNAL_ERROR();
					   }

					   auto tempBlock = BasicBlock::Create( TheContext, "temp", static_cast<Function *>( currentFunction->get() ) );
					   Builder.SetInsertPoint( tempBlock );

					   return VoidType();
				   } },
				  { "goto", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto labelName = children[ 1 ][ 1 ].asString();
					   auto targetLable = labelJump.find( labelName );
					   if ( targetLable != labelJump.end() )
					   {
						   Builder.CreateBr( targetLable->second );
					   }
					   else
					   {
						   gotoJump[ labelName ].emplace_back(
							 Builder.GetInsertBlock(),
							 children[ 1 ] );
					   }

					   auto tempBlock = BasicBlock::Create( TheContext, "temp", static_cast<Function *>( currentFunction->get() ) );
					   Builder.SetInsertPoint( tempBlock );
					   return VoidType();
				   } },
				  { "continue", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   if ( continueJump.empty() )
					   {
						   infoList->add_msg(
							 MSG_TYPE_ERROR,
							 fmt( "`continue` statement not in loop statement" ),
							 ast );
						   HALT();
					   }

					   auto targetBB = continueJump.top();
					   Builder.CreateBr( targetBB );

					   auto tempBlock = BasicBlock::Create( TheContext, "temp", static_cast<Function *>( currentFunction->get() ) );
					   Builder.SetInsertPoint( tempBlock );

					   return VoidType();
				   } },
				  { "break", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   if ( breakJump.empty() )
					   {
						   infoList->add_msg(
							 MSG_TYPE_ERROR,
							 fmt( "`break` statement not in loop or switch statement" ),
							 ast );
						   HALT();
					   }
					   auto targetBB = breakJump.top();
					   Builder.CreateBr( targetBB );

					   auto tempBlock = BasicBlock::Create( TheContext, "temp", static_cast<Function *>( currentFunction->get() ) );
					   Builder.SetInsertPoint( tempBlock );

					   return VoidType();
				   } }
			  };
			  if ( __.find( typeRet ) != __.end() )
			  {
				  return __[ typeRet ]( children, node );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		{ "selection_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto stateType = children[ 0 ][ 1 ].asString();
			  if ( stateType == "if" )
			  {
				  if ( children.size() == 7 )
				  {
					  auto func = currentFunction->get();
					  auto ifEnd = BasicBlock::Create( TheContext, "if.end", static_cast<Function *>( func ) );
					  auto ifElse = BasicBlock::Create( TheContext, "if.else", static_cast<Function *>( func ), ifEnd );
					  auto ifThen = BasicBlock::Create( TheContext, "if.then", static_cast<Function *>( func ), ifElse );

					  auto br = get<QualifiedValue>( codegen( children[ 2 ] ) )
								  .value( children[ 2 ] )
								  .cast( TypeView::getBoolTy(), children[ 2 ] );
					  Builder.CreateCondBr( br.get(), ifThen, ifElse );

					  Builder.SetInsertPoint( ifThen );
					  codegen( children[ 4 ] );
					  Builder.CreateBr( ifEnd );

					  Builder.SetInsertPoint( ifElse );
					  codegen( children[ 6 ] );
					  Builder.CreateBr( ifEnd );

					  Builder.SetInsertPoint( ifEnd );
				  }
				  else if ( children.size() == 5 )
				  {
					  auto func = currentFunction->get();
					  auto ifEnd = BasicBlock::Create( TheContext, "if.end", static_cast<Function *>( func ) );
					  auto ifThen = BasicBlock::Create( TheContext, "if.then", static_cast<Function *>( func ), ifEnd );

					  auto br = get<QualifiedValue>( codegen( children[ 2 ] ) )
								  .value( children[ 2 ] )
								  .cast( TypeView::getBoolTy(), children[ 2 ] );
					  Builder.CreateCondBr( br.get(), ifThen, ifEnd );

					  Builder.SetInsertPoint( ifThen );
					  codegen( children[ 4 ] );
					  Builder.CreateBr( ifEnd );

					  Builder.SetInsertPoint( ifEnd );
				  }
				  else
				  {
					  INTERNAL_ERROR();
				  }

				  return VoidType();
			  }
			  else if ( stateType == "switch" )
			  {
				  auto value = get<QualifiedValue>( codegen( children[ 2 ] ) )
								 .value( children[ 2 ] );
				  auto &valType = value.get_type();
				  //

				  int bits;
				  if ( auto intType = valType->as<mty::Integer>() )
				  {
					  bits = intType->bits;
				  }
				  else
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "statement requires expression of integer type" ),
						node );
					  HALT();
				  }

				  if ( bits < 32 )
				  {
					  value.cast( TypeView::getIntTy( true ), children[ 2 ] );
				  }

				  auto epilog = BasicBlock::Create( TheContext, "sw.epilog", static_cast<Function *>( currentFunction->get() ) );
				  BasicBlock *defaultCase = nullptr;

				  auto switchIns = Builder.CreateSwitch( value.get(), epilog );

				  switchBits.emplace( bits );
				  caseList.emplace( std::map<ConstantInt *, BasicBlock *>() );
				  defaultList.emplace( std::make_pair( false, defaultCase ) );
				  breakJump.emplace( epilog );

				  codegen( children[ 4 ] );

				  auto &defaultTarget = defaultList.top();
				  if ( defaultTarget.first )
				  {
					  switchIns->setDefaultDest( defaultTarget.second );
				  }

				  auto &cases = caseList.top();
				  for ( auto cs : cases )
				  {
					  switchIns->addCase( cs.first, cs.second );
				  }

				  caseList.pop();
				  defaultList.pop();
				  breakJump.pop();
				  switchBits.pop();

				  Builder.CreateBr( epilog );
				  Builder.SetInsertPoint( epilog );

				  return VoidType();
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		{ "labeled_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto labelType = children[ 0 ][ 1 ].asString();
			  if ( labelType == "case" )
			  {
				  auto value = get<QualifiedValue>( codegen( children[ 1 ] ) )
								 .value( children[ 1 ] );
				  //  .cast( TypeView::getIntTy( true ), children[ 1 ] );

				  if ( !value.get_type()->is<mty::Integer>() )
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "expression is not an integer constant expression" ),
						node );
					  HALT();
				  }

				  int bits = switchBits.top();
				  if ( bits == 64 )
				  {
					  value.cast( TypeView::getLongLongTy( true ), children[ 1 ] );
				  }
				  else
				  {
					  value.cast( TypeView::getIntTy( true ), children[ 1 ] );
				  }

				  ConstantInt *cc_val = dyn_cast_or_null<ConstantInt>( value.get() );
				  if ( !cc_val )
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "expression is not an integer constant expression" ),
						node );
					  HALT();
				  }

				  auto castBlock = BasicBlock::Create( TheContext, "sw.bb", static_cast<Function *>( currentFunction->get() ) );

				  auto &cases = caseList.top();
				  //   cases[ cc_val ] = castBlock;
				  cases.insert( std::make_pair( cc_val, castBlock ) );  // redefinition

				  auto bb = Builder.GetInsertBlock();
				  auto inst = &bb->back();
				  if ( !dyn_cast_or_null<SwitchInst>( inst ) )
				  {
					  Builder.CreateBr( castBlock );
				  }

				  Builder.SetInsertPoint( castBlock );
				  codegen( children[ 3 ] );

				  return VoidType();
			  }
			  else if ( labelType == "default" )
			  {
				  auto &defaultBlock = defaultList.top();
				  defaultBlock.first = true;

				  defaultBlock.second = BasicBlock::Create( TheContext, "sw.default", static_cast<Function *>( currentFunction->get() ) );

				  auto bb = Builder.GetInsertBlock();
				  auto inst = &bb->back();
				  if ( !dyn_cast_or_null<SwitchInst>( inst ) )
				  {
					  Builder.CreateBr( defaultBlock.second );
				  }

				  Builder.SetInsertPoint( defaultBlock.second );
				  codegen( children[ 2 ] );

				  return VoidType();
			  }
			  else
			  {
				  auto label = BasicBlock::Create( TheContext, labelType, static_cast<Function *>( currentFunction->get() ) );
				  Builder.CreateBr( label );

				  if ( labelJump.find( labelType ) != labelJump.end() )
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "redefinition of label `", labelType, "`" ),
						node );
					  HALT();
				  }
				  labelJump[ labelType ] = label;

				  if ( gotoJump.find( labelType ) != gotoJump.end() )
				  {
					  for ( auto &entry : gotoJump[ labelType ] )
					  {
						  Builder.SetInsertPoint( entry.first );
						  Builder.CreateBr( label );
					  }
					  gotoJump.erase( labelType );
				  }

				  Builder.SetInsertPoint( label );
				  codegen( children[ 2 ] );
			  }
			  return VoidType();
		  } ) }
	};

	handlers.insert( stmt.begin(), stmt.end() );

	return 0;
}
