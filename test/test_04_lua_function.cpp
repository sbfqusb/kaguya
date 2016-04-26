#include "test_util.hpp"
#include "kaguya/kaguya.hpp"


KAGUYA_TEST_GROUP_START(test_04_lua_function)
using namespace kaguya_test_util;
using namespace kaguya::standard;

KAGUYA_TEST_FUNCTION_DEF(function_call)(kaguya::State& state)
{
	state("testfun = function(a,b,c,d,e) assert(a == 1,a) assert(b == 2,b) assert(c == 4,c) assert(d == 8,d) assert(e == 16,e) end");
	state["testfun"](1,2,4,8,16);
	state("testfun2 = function() return 3232 end");

	TEST_EQUAL(state["testfun2"](), 3232);
	state("testfun3 = function() return 'text' end");
	TEST_EQUAL(state["testfun3"](), "text");
}

KAGUYA_TEST_FUNCTION_DEF(multi_return_function_test)(kaguya::State& state)
{
	state("multresfun =function() return 1,2,4,8,16 end");
	int a, b, c, d, e;
	a = b = c = d = e = 0;
	kaguya::tie(a, b, c, d, e) = state["multresfun"]();
	TEST_EQUAL(a, 1);
	TEST_EQUAL(b, 2);
	TEST_EQUAL(c, 4);
	TEST_EQUAL(d, 8);
	TEST_EQUAL(e, 16);

	a = b = c = d = e = 0;
	kaguya::tie(a, b, c, d, e) = state["multresfun"].call<tuple<int, int, int, int, int> >();
	TEST_EQUAL(a, 1);
	TEST_EQUAL(b, 2);
	TEST_EQUAL(c, 4);
	TEST_EQUAL(d, 8);
	TEST_EQUAL(e, 16);

	using kaguya::standard::get;
	tuple<int, int, int, int, int> tuple_res = state["multresfun"].call<tuple<int, int, int, int, int> >();
	TEST_EQUAL(get<0>(tuple_res), 1);
	TEST_EQUAL(get<1>(tuple_res), 2);
	TEST_EQUAL(get<2>(tuple_res), 4);
	TEST_EQUAL(get<3>(tuple_res), 8);
	TEST_EQUAL(get<4>(tuple_res), 16);

	state("multireturn_pass_through_to_arg = function(a,b,c,d,e) assert(a == 1,a) assert(b == 2,b) assert(c == 4,c) assert(d == 8,d) assert(e == 16,e) end");
	state["multireturn_pass_through_to_arg"](state["multresfun"].call<tuple<int, int, int, int, int> >());
	state["multireturn_pass_through_to_arg"](state["multresfun"]());
}


KAGUYA_TEST_FUNCTION_DEF(coroutine)(kaguya::State& state)
{
	{
		TEST_CHECK(state("cor = coroutine.create( function()"
			"coroutine.yield(32) "
			"coroutine.yield(53) "
			"return 2 "
			" end)"));

		kaguya::LuaRef cor = state["cor"];
		int r1 = cor();
		int r2 = cor();
		int r3 = cor();

		TEST_EQUAL(r1, 32);
		TEST_EQUAL(r2, 53);
		TEST_EQUAL(r3, 2);

		TEST_CHECK(state("cor3 = coroutine.create( function(arg)"
			"coroutine.yield(arg) "
			"coroutine.yield(arg*2) "
			"coroutine.yield(arg*3) "
			"return arg*4 "
			" end)"));

		kaguya::LuaThread cor3 = state["cor3"];
		std::vector<int> results;
		while (!cor3.isThreadDead())
		{
			results.push_back(cor3(3));
		}

		TEST_EQUAL(results.size(), 4);
		TEST_EQUAL(results[0], 3);
		TEST_EQUAL(results[1], 6);
		TEST_EQUAL(results[2], 9);
		TEST_EQUAL(results[3], 12);
	}
	{
		kaguya::LuaThread cor2 = state.newRef(kaguya::NewThread());
		TEST_CHECK(state("corfun = function(arg)"
			"coroutine.yield(arg) "
			"coroutine.yield(arg*2) "
			"coroutine.yield(arg*3) "
			"return arg*4 "
			" end"));

		kaguya::LuaFunction corfun = state["corfun"];
		int r1 = cor2(corfun, 3);
		int r2 = cor2.resume<int>();
		int r3 = cor2.resume<int>();
		int r4 = cor2.resume<int>();

		TEST_EQUAL(r1, 3);
		TEST_EQUAL(r2, 6);
		TEST_EQUAL(r3, 9);
		TEST_EQUAL(r4, 12);


		cor2.resume<void>(corfun, 3);
	}
	{

		state["cor2"] = kaguya::NewThread();
		kaguya::LuaRef cor2 = state["cor2"];
		TEST_CHECK(state("corfun = function(arg)"
			"for i = 1,arg do "
			"coroutine.yield() "
			"end "
			"end"));
		kaguya::LuaRef corfun = state["corfun"];
		cor2(corfun, 10);
		int yieldnum = 0;
		while (cor2.threadStatus() == LUA_YIELD)
		{
			cor2();
			yieldnum++;
		}


		TEST_EQUAL(yieldnum, 10);
	}
}

KAGUYA_TEST_FUNCTION_DEF(coroutine_dead)(kaguya::State& state)
{
	kaguya::LuaThread emptycoroutine;
	TEST_CHECK(emptycoroutine.isThreadDead());

	TEST_CHECK(state("corfun = function(arg)"
		"return 1 "
		" end"));
	kaguya::LuaThread cor = state.newThread(state["corfun"]);

	TEST_CHECK(!cor.isThreadDead());
	cor();
	TEST_CHECK(cor.isThreadDead());
}

void corresult_to_main(kaguya::VariadicArgType args)
{
	TEST_EQUAL(args.size(), 9);
	TEST_EQUAL(args[0], 1);
	TEST_EQUAL(args[1], 2);
	TEST_EQUAL(args[2], 3);
	TEST_EQUAL(args[3], 4);
	TEST_EQUAL(args[4], 5);
	TEST_EQUAL(args[5], 6);
	TEST_EQUAL(args[6], 7);
	TEST_EQUAL(args[7], 8);
	TEST_EQUAL(args[8], 9);
}
void corresult_to_main2(kaguya::VariadicArgType args)
{
	TEST_EQUAL(args.size(), 1);
	TEST_EQUAL(args[0], 6);
}


KAGUYA_TEST_FUNCTION_DEF(coroutine_stack)(kaguya::State& state)
{
	state["corresult_to_main"] = &corresult_to_main;
	state["cor2"] = kaguya::NewThread();
	kaguya::LuaRef cor2 = state["cor2"];
	TEST_CHECK(state("corfun = function(arg)"
		"for i = 1,arg do "
		"coroutine.yield(1,2,3,4,5,6,7,8,9) "
		"end "
		"end"));
	state["corresult_to_main"](cor2(state["corfun"], 10));


	state["corresult_to_main2"] = &corresult_to_main2;
	state["corresult_to_main2"](cor2(state["corfun"], 10).result_at(5));


	TEST_CHECK(state("corfun = function(arg)"
		"for i = 1,arg do "
		"coroutine.yield({value=323}) "
		"end "
		"end"));


	state["cor3"] = kaguya::NewThread();
	state["value"] = state["cor3"](state["corfun"], 10).getField("value");
	TEST_EQUAL(state["value"], 323);
	TEST_CHECK(state["value"]);
	TEST_CHECK(state("assert(value == 323)"));
	TEST_EQUAL(state["value"], state["cor3"](state["corfun"], 10).getField<int>("value"));
}

#if (defined(_MSC_VER) && _MSC_VER < 1900)//for MSVC2013 bug
void luacallback(const function<int(float)>& callback)
{
	callback(32.f);
}
#else
void luacallback(const function<void(float)>& callback)
{
	callback(32.f);
}
#endif

KAGUYA_TEST_FUNCTION_DEF(bind_to_std_function)(kaguya::State& state)
{
	state["luacallback"] = &luacallback;
	state("luacallback(function(v) assert(32 == v) end)");
}

KAGUYA_TEST_GROUP_END(test_04_lua_function)
