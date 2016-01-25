/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "boolean_graph.h"

#include <gtest/gtest.h>

#include "initializer.h"
#include "fault_tree.h"
#include "model.h"
#include "settings.h"

namespace scram {
namespace test {

TEST(BooleanGraphTest, Print) {
  Settings settings;
  Initializer* init = new Initializer(settings);
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/fta/correct_formulas.xml");
  EXPECT_NO_THROW(init->ProcessInputFiles(input_files));
  BooleanGraph* graph = new BooleanGraph(init->model()
                                         ->fault_trees().begin()->second
                                         ->top_events().front());
  graph->Print();
  delete init;
  delete graph;
}

static_assert(kNumOperators == 8, "New gate types are not considered!");

class IGateTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    var_one = std::make_shared<Variable>();
    var_two = std::make_shared<Variable>();
    var_three = std::make_shared<Variable>();
    vars_ = {var_one, var_two, var_three};
    for (int i = 0; i < 2; ++i) vars_.emplace_back(new Variable());  // Extra.
  }

  virtual void TearDown() {
    Node::ResetIndex();
    Variable::ResetIndex();
  }

  /// Sets up the main gate with the default variables.
  ///
  /// @param[in] type  Type for the main gate.
  /// @param[in] num_vars  Desired number of variables.
  ///
  /// @note The setup is not for one-arg gates (NOT/NULL).
  /// @note For K/N gates, K is set to 2 by default.
  void DefineGate(Operator type, int num_vars) {
    assert(num_vars < 6);
    assert(!(type == kAtleastGate && num_vars < 2));

    g = std::make_shared<IGate>(type);
    if (type == kAtleastGate) g->vote_number(2);
    for (int i = 0; i < num_vars; ++i) g->AddArg(vars_[i]->index(), vars_[i]);

    assert(g->state() == kNormalState);
    assert(g->type() == type);
    assert(g->args().size() == num_vars);
    assert(g->variable_args().size() == num_vars);
    assert(g->gate_args().empty());
    assert(g->constant_args().empty());
  }

  IGatePtr g;  // Main gate for manipulations.
  // Collection of variables for gate input.
  VariablePtr var_one;
  VariablePtr var_two;
  VariablePtr var_three;

 private:
  std::vector<VariablePtr> vars_;  // For convenience only.
};

#ifndef NDEBUG
TEST_F(IGateTest, AddArgDeathTests) {
  DefineGate(kNullGate, 1);
  EXPECT_DEATH(g->AddArg(var_two->index(), var_two), "");
  DefineGate(kNotGate, 1);
  EXPECT_DEATH(g->AddArg(var_two->index(), var_two), "");
  DefineGate(kXorGate, 2);
  EXPECT_DEATH(g->AddArg(var_three->index(), var_three), "");
  DefineGate(kAndGate, 1);
  EXPECT_DEATH(g->AddArg(1, var_two), "");  // Wrong index.
  DefineGate(kAndGate, 1);
  EXPECT_DEATH(g->AddArg(0, var_two), "");  // Wrong index.
  DefineGate(kAndGate, 2);
  g->Nullify();  // Constant state.
  EXPECT_DEATH(g->AddArg(var_two->index(), var_two), "");  // Wrong index.
  DefineGate(kAtleastGate, 3);
  g->vote_number(-1);  // Negative vote number.
  EXPECT_DEATH(g->AddArg(var_three->index(), var_three), "");
}
#endif

/// Collection of tests
/// for addition of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Operator, i.e., 'kAndGate'.
/// @param num_vars  The number of variables to initialize the gate.
#define ADD_ARG_IGNORE_TEST(short_type, num_vars) \
  DefineGate(k##short_type##Gate, num_vars);      \
  g->AddArg(var_one->index(), var_one);           \
  ASSERT_EQ(kNormalState, g->state());            \
  EXPECT_EQ(num_vars, g->args().size());          \
  EXPECT_EQ(num_vars, g->variable_args().size()); \
  EXPECT_TRUE(g->gate_args().empty());            \
  EXPECT_TRUE(g->constant_args().empty())

/// Tests addition of an existing argument to Boolean graph gates
/// that do not change the type of the gate.
///
/// @param short_type  Short name of the gate type, i.e., 'And'.
#define TEST_DUP_ARG_IGNORE(short_type)               \
  TEST_F(IGateTest, DuplicateArgIgnore##short_type) { \
    ADD_ARG_IGNORE_TEST(short_type, 2);               \
    EXPECT_EQ(k##short_type##Gate, g->type());        \
  }

TEST_DUP_ARG_IGNORE(And);
TEST_DUP_ARG_IGNORE(Or);
TEST_DUP_ARG_IGNORE(Nand);
TEST_DUP_ARG_IGNORE(Nor);

#undef TEST_DUP_ARG_IGNORE

/// Tests addition of an existing argument
/// that changes the type of the gate.
///
/// @param init_type  Short name of the initial type of the gate, i.e., 'And'.
/// @param final_type  The resulting type of addition operation.
#define TEST_DUP_ARG_TYPE_CHANGE(init_type, final_type)    \
  TEST_F(IGateTest, DuplicateArgChange##init_type##Type) { \
    ADD_ARG_IGNORE_TEST(init_type, 1);                     \
    EXPECT_EQ(k##final_type##Gate, g->type());             \
  }

TEST_DUP_ARG_TYPE_CHANGE(Or, Null);
TEST_DUP_ARG_TYPE_CHANGE(And, Null);
TEST_DUP_ARG_TYPE_CHANGE(Nor, Not);
TEST_DUP_ARG_TYPE_CHANGE(Nand, Not);

#undef TEST_DUP_ARG_TYPE_CHANGE
#undef ADD_ARG_IGNORE_TEST

TEST_F(IGateTest, DuplicateArgXor) {
  DefineGate(kXorGate, 1);
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNullState, g->state());
  EXPECT_TRUE(g->args().empty());
}

TEST_F(IGateTest, DuplicateArgAtleastToNull) {
  DefineGate(kAtleastGate, 2);
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNormalState, g->state());
  EXPECT_EQ(kNullGate, g->type());
  EXPECT_EQ(1, g->args().size());
  EXPECT_EQ(var_two->index(), g->variable_args().begin()->first);
}

TEST_F(IGateTest, DuplicateArgAtleastToAnd) {
  DefineGate(kAtleastGate, 3);
  g->vote_number(3);  // K equals to the number of input arguments.
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNormalState, g->state());
  EXPECT_EQ(kAndGate, g->type());
  EXPECT_EQ(2, g->args().size());
  ASSERT_EQ(1, g->variable_args().size());
  EXPECT_EQ(var_one->index(), g->variable_args().begin()->first);
  ASSERT_EQ(1, g->gate_args().size());

  IGatePtr sub = g->gate_args().begin()->second;
  EXPECT_EQ(kOrGate, sub->type());  // Special case. K/N is in general.
  EXPECT_EQ(1, sub->vote_number());  // This is the reason.
  std::set<int> vars = {var_two->index(), var_three->index()};
  EXPECT_EQ(vars, sub->args());
  EXPECT_EQ(2, sub->variable_args().size());
}

TEST_F(IGateTest, DuplicateArgAtleastToOrWithOneClone) {
  DefineGate(kAtleastGate, 3);
  g->vote_number(2);
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNormalState, g->state());
  EXPECT_EQ(kOrGate, g->type());
  EXPECT_EQ(2, g->args().size());
  ASSERT_EQ(1, g->variable_args().size());
  EXPECT_EQ(var_one->index(), g->variable_args().begin()->first);
  ASSERT_EQ(1, g->gate_args().size());

  IGatePtr sub = g->gate_args().begin()->second;
  EXPECT_EQ(kAndGate, sub->type());  // Special case. K/N is in general.
  EXPECT_EQ(2, sub->vote_number());
  EXPECT_EQ(2, sub->args().size());  // This is the reason.
  std::set<int> vars = {var_two->index(), var_three->index()};
  EXPECT_EQ(vars, sub->args());
  EXPECT_EQ(2, sub->variable_args().size());
}

TEST_F(IGateTest, DuplicateArgAtleastToOrWithTwoClones) {
  DefineGate(kAtleastGate, 5);
  g->vote_number(3);
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNormalState, g->state());
  EXPECT_EQ(kOrGate, g->type());
  EXPECT_EQ(2, g->args().size());
  EXPECT_TRUE(g->variable_args().empty());
  ASSERT_EQ(2, g->gate_args().size());
  auto it = g->gate_args().begin();
  IGatePtr and_gate = it->second;  // Guessing.
  ++it;
  IGatePtr clone_one = it->second;  // Guessing.
  // Correcting the guess.
  if (and_gate->type() != kAndGate) std::swap(and_gate, clone_one);
  ASSERT_EQ(kAndGate, and_gate->type());
  ASSERT_EQ(kAtleastGate, clone_one->type());

  EXPECT_EQ(kNormalState, clone_one->state());
  EXPECT_EQ(3, clone_one->vote_number());
  EXPECT_EQ(4, clone_one->args().size());
  EXPECT_EQ(4, clone_one->variable_args().size());

  EXPECT_EQ(kNormalState, and_gate->state());
  EXPECT_EQ(2, and_gate->args().size());
  ASSERT_EQ(1, and_gate->variable_args().size());
  EXPECT_EQ(var_one->index(), and_gate->variable_args().begin()->first);
  ASSERT_EQ(1, and_gate->gate_args().size());

  IGatePtr clone_two = and_gate->gate_args().begin()->second;
  EXPECT_EQ(kNormalState, clone_two->state());
  EXPECT_EQ(kOrGate, clone_two->type());  // Special case. K/N is in general.
  EXPECT_EQ(1, clone_two->vote_number());  // This is the reason.
  EXPECT_EQ(4, clone_two->args().size());
  EXPECT_EQ(4, clone_two->variable_args().size());
}

/// Collection of tests
/// for addition of the complement of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Operator, i.e., 'kAndGate'.
/// @param const_set  The notion of constant set (Null, Unity).
#define TEST_ADD_COMPLEMENT_ARG(short_type, const_set) \
  TEST_F(IGateTest, ComplementArg##short_type) {       \
    DefineGate(k##short_type##Gate, 1);                \
    g->AddArg(-var_one->index(), var_one);             \
    ASSERT_EQ(k##const_set##State, g->state());        \
    EXPECT_TRUE(g->args().empty());                    \
    EXPECT_TRUE(g->variable_args().empty());           \
    EXPECT_TRUE(g->gate_args().empty());               \
    EXPECT_TRUE(g->constant_args().empty());           \
  }

TEST_ADD_COMPLEMENT_ARG(And, Null);
TEST_ADD_COMPLEMENT_ARG(Or, Unity);
TEST_ADD_COMPLEMENT_ARG(Nand, Unity);
TEST_ADD_COMPLEMENT_ARG(Nor, Null);
TEST_ADD_COMPLEMENT_ARG(Xor, Unity);

#undef TEST_ADD_COMPLEMENT_ARG

/// Collection of ATLEAST (K/N) gate tests
/// for addition of the complement of an existing argument.
///
/// @param num_vars  Initial number of variables.
/// @param v_num  Initial K number of the gate.
/// @param final_type  Short name of the final type of the gate, i.e., 'And'.
#define TEST_ADD_COMPLEMENT_ARG_KN(num_vars, v_num, final_type) \
  TEST_F(IGateTest, ComplementArgAtleastTo##final_type) {       \
    DefineGate(kAtleastGate, num_vars);                         \
    g->vote_number(v_num);                                      \
    g->AddArg(-var_one->index(), var_one);                      \
    ASSERT_EQ(kNormalState, g->state());                        \
    EXPECT_EQ(k##final_type##Gate, g->type());                  \
    EXPECT_EQ(num_vars - 1, g->args().size());                  \
    EXPECT_EQ(num_vars - 1, g->variable_args().size());         \
    EXPECT_EQ(v_num - 1, g->vote_number());                     \
    EXPECT_TRUE(g->gate_args().empty());                        \
    EXPECT_TRUE(g->constant_args().empty());                    \
  }

TEST_ADD_COMPLEMENT_ARG_KN(2, 2, Null);  // Join operation.
TEST_ADD_COMPLEMENT_ARG_KN(3, 2, Or);  // General case.
TEST_ADD_COMPLEMENT_ARG_KN(3, 3, And);  // Join operation.

#undef TEST_ADD_COMPLEMENT_ARG_KN

/// Tests for processing of a constant argument of a gate,
/// which results in gate becoming constant itself.
///
/// @param arg_state  The true or false state of the gate argument.
/// @param num_vars  The initial number of gate arguments.
/// @param init_type  The initial type of the gate.
/// @param const_set  The notion of constant set (Null, Unity) as gate's state.
#define TEST_CONSTANT_ARG_STATE(arg_state, num_vars, init_type, const_set) \
  TEST_F(IGateTest, arg_state##ConstantArg##init_type) {                   \
    DefineGate(k##init_type##Gate, num_vars);                              \
    g->ProcessConstantArg(var_one, arg_state);                             \
    EXPECT_EQ(k##const_set##State, g->state());                            \
    EXPECT_TRUE(g->args().empty());                                        \
    EXPECT_TRUE(g->variable_args().empty());                               \
    EXPECT_TRUE(g->gate_args().empty());                                   \
    EXPECT_TRUE(g->constant_args().empty());                               \
  }

TEST_CONSTANT_ARG_STATE(true, 1, Null, Unity);
TEST_CONSTANT_ARG_STATE(false, 1, Null, Null);
TEST_CONSTANT_ARG_STATE(false, 1, Not, Unity);
TEST_CONSTANT_ARG_STATE(true, 1, Not, Null);
TEST_CONSTANT_ARG_STATE(true, 2, Or, Unity);
TEST_CONSTANT_ARG_STATE(false, 2, And, Null);
TEST_CONSTANT_ARG_STATE(true, 2, Nor, Null);
TEST_CONSTANT_ARG_STATE(false, 2, Nand, Unity);

#undef TEST_CONSTANT_ARG_STATE

/// Tests for processing of a constant argument of a gate,
/// which results in type change of the gate.
///
/// @param arg_state  The true or false state of the gate argument.
/// @param num_vars  The initial number of gate arguments.
/// @param v_num  The initial vote number of the gate.
/// @param init_type  The initial type of the gate.
/// @param final_type  The final type of the gate.
#define TEST_CONSTANT_ARG_VNUM(arg_state, num_vars, v_num, init_type,    \
                               final_type)                               \
  TEST_F(IGateTest, arg_state##ConstantArg##init_type##To##final_type) { \
    DefineGate(k##init_type##Gate, num_vars);                            \
    if (v_num) g->vote_number(v_num);                                    \
    g->ProcessConstantArg(var_one, arg_state);                           \
    ASSERT_EQ(kNormalState, g->state());                                 \
    EXPECT_EQ(k##final_type##Gate, g->type());                           \
    EXPECT_EQ(num_vars - 1, g->args().size());                           \
    EXPECT_EQ(num_vars - 1, g->variable_args().size());                  \
    EXPECT_TRUE(g->gate_args().empty());                                 \
    EXPECT_TRUE(g->constant_args().empty());                             \
  }

TEST_CONSTANT_ARG_VNUM(true, 3, 2, Atleast, Or);
TEST_CONSTANT_ARG_VNUM(true, 4, 3, Atleast, Atleast);
TEST_CONSTANT_ARG_VNUM(false, 3, 2, Atleast, And);
TEST_CONSTANT_ARG_VNUM(false, 4, 2, Atleast, Atleast);

/// The same tests as TEST_CONSTANT_ARG_VNUM
/// but with no vote number initialization.
#define TEST_CONSTANT_ARG(arg_state, num_vars, init_type, final_type) \
  TEST_CONSTANT_ARG_VNUM(arg_state, num_vars, false, init_type, final_type)

TEST_CONSTANT_ARG(false, 2, Or, Null);
TEST_CONSTANT_ARG(false, 3, Or, Or);
TEST_CONSTANT_ARG(true, 2, And, Null);
TEST_CONSTANT_ARG(true, 3, And, And);
TEST_CONSTANT_ARG(false, 2, Nor, Not);
TEST_CONSTANT_ARG(false, 3, Nor, Nor);
TEST_CONSTANT_ARG(true, 2, Nand, Not);
TEST_CONSTANT_ARG(true, 3, Nand, Nand);
TEST_CONSTANT_ARG(true, 2, Xor, Not);
TEST_CONSTANT_ARG(false, 2, Xor, Null);

#undef TEST_CONSTANT_ARG_VNUM
#undef TEST_CONSTANT_ARG

}  // namespace test
}  // namespace scram
