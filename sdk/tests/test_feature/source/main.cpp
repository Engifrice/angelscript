#include <stdio.h>
#if defined(WIN32) || defined(_WIN64)
#include <conio.h>
#endif
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#ifdef __dreamcast__
#include <kos.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

#endif

bool TestCreateEngine();
bool TestExecute();
bool TestExecute1Arg();
bool TestExecute2Args();
bool TestExecute4Args();
bool TestExecute4Argsf();
bool TestExecute32Args();
bool TestExecuteMixedArgs();
bool TestExecute32MixedArgs();
bool TestExecuteThis32MixedArgs();
bool TestTempVar();
bool TestExecuteScript();
bool Test2Modules();
bool TestStdcall4Args();
bool TestInt64();
bool TestModuleRef();
bool TestEnumGlobVar();
bool TestGlobalVar();
bool TestBStr();
bool TestSwitch();
bool TestNegateOperator();
bool TestCDecl_Class();
bool TestCDecl_ClassA();
bool TestCDecl_ClassC();
bool TestCDecl_ClassD();
bool TestCDecl_ClassK();
bool TestThiscallClass();
bool TestNotComplexThisCall();
bool TestNotComplexStdcall();
bool TestReturnWithCDeclObjFirst();
bool TestStdString();
bool TestLongToken();
bool TestVirtualMethod();
bool TestMultipleInheritance();
bool TestVirtualInheritance();
bool TestStack();
bool TestExecuteString();
bool TestCondition();
bool TestNeverVisited();
bool TestNested();
bool TestOptimize();
bool TestNotInitialized();
bool TestVector3();

namespace TestLiteral           { bool Test(); }
namespace TestForEach           { bool Test(); }
namespace TestException         { bool Test(); }
namespace TestCDeclReturn       { bool Test(); }
namespace TestCustomMem         { bool Test(); }
namespace TestGeneric           { bool Test(); }
namespace TestDebug             { bool Test(); }
namespace TestSuspend           { bool Test(); }
namespace TestConstProperty     { bool Test(); }
namespace TestConstObject       { bool Test(); }
namespace TestOutput            { bool Test(); }
namespace TestImport            { bool Test(); }
namespace TestImport2           { bool Test(); }
namespace Test2Func             { bool Test(); }
namespace TestDiscard           { bool Test(); }
namespace TestCircularImport    { bool Test(); }
namespace TestMultiAssign       { bool Test(); }
namespace TestSaveLoad          { bool Test(); }
namespace TestConstructor2      { bool Test(); }
namespace TestContext           { bool Test(); }
namespace TestArray             { bool Test(); }
namespace TestArrayHandle       { bool Test(); }
namespace TestStdVector         { bool Test(); }
namespace TestArrayObject       { bool Test(); }
namespace TestPointer           { bool Test(); }
namespace TestConversion        { bool Test(); }
namespace TestObject            { bool Test(); }
namespace TestObject2           { bool Test(); }
namespace TestObject3           { bool Test(); }
namespace TestExceptionMemory   { bool Test(); }
namespace TestArgRef            { bool Test(); }
namespace TestObjHandle         { bool Test(); }
namespace TestObjHandle2        { bool Test(); }
namespace TestObjZeroSize       { bool Test(); }
namespace TestRefArgument       { bool Test(); }
namespace TestStack2            { bool Test(); }
namespace TestScriptString      { bool Test(); }
namespace TestScriptStruct      { bool Test(); }
namespace TestStructIntf        { bool Test(); }
namespace TestAutoHandle        { bool Test(); }
namespace TestAny               { bool Test(); }
namespace TestArrayIntf         { bool Test(); }
namespace TestDynamicConfig     { bool Test(); }
namespace TestStream            { bool Test(); }
namespace TestConfig            { bool Test(); }
namespace TestConfigAccess      { bool Test(); }
namespace TestFloat             { bool Test(); }
namespace TestVector3_2         { bool Test(); }
namespace TestDict              { bool Test(); }
namespace TestUnsafeRef         { bool Test(); }
namespace TestReturnString      { bool Test(); }
namespace TestScriptClassMethod { bool Test(); }
namespace TestPostProcess       { bool Test(); }
namespace TestShark             { bool Test(); }
namespace TestParser            { bool Test(); }
namespace TestInterface         { bool Test(); }
namespace TestCompiler          { bool Test(); }
namespace TestSingleton         { bool Test(); }
namespace TestCastOp            { bool Test(); }
namespace TestFor               { bool Test(); }
namespace TestBits              { bool Test(); }
namespace TestGetArgPtr         { bool Test(); }
namespace TestCString           { bool Test(); }
namespace TestBool              { bool Test(); }
namespace TestInt8              { bool Test(); }
namespace TestScriptMath        { bool Test(); }
namespace TestVarType           { bool Test(); }
namespace TestDestructor        { bool Test(); }
namespace TestRegisterType      { bool Test(); }
namespace TestFactory           { bool Test(); }
namespace TestRZ                { bool Test(); }
namespace TestImplicitCast      { bool Test(); }
namespace TestAssign            { bool Test(); }
namespace TestTypedef           { bool Test(); }
namespace TestEnum              { bool Test(); }
namespace TestRefCast           { bool Test(); }
namespace TestImplicitHandle    { bool Test(); }
namespace TestAuto              { bool Test(); }
namespace TestInheritance       { bool Test(); }
namespace TestDump              { bool Test(); }
namespace TestTemplate          { bool Test(); }
namespace TestOperator          { bool Test(); }
namespace TestGetSet            { bool Test(); }
namespace TestModule            { bool Test(); }
namespace TestFunctionPtr       { bool Test(); }
namespace TestGarbageCollect    { bool Test(); }
namespace TestScriptRetRef      { bool Test(); }
namespace TestDefaultArg        { bool Test(); }
namespace TestNamedArgs         { bool Test(); }
namespace TestShared            { bool Test(); }
namespace TestNamespace         { bool Test(); }
namespace TestCDeclObjLast      { bool Test(); }
namespace TestMixin             { bool Test(); }
namespace TestThiscallAsGlobal  { bool Test(); }
namespace TestPow               { bool Test(); }
namespace TestThisCallMethod    { bool Test(); }
namespace TestThisCallMethod_ConfigErrors { bool Test(); }
namespace TestPropIntegerDivision         { bool Test(); }
namespace TestComposition                 { bool Test(); }
namespace Test_Native_DefaultFunc         { bool Test(); }
namespace TestFuncOverload                { bool Test(); }
namespace TestConstructor                 { bool Test(); }

namespace Test_Addon_Autowrapper   { bool Test(); }
namespace Test_Addon_ScriptArray   { bool Test(); }
namespace Test_Addon_ScriptHandle  { bool Test(); }
namespace Test_Addon_Serializer    { bool Test(); }
namespace Test_Addon_ScriptMath    { bool Test(); }
namespace Test_Addon_ScriptBuilder { bool Test(); }
namespace Test_Addon_Dictionary    { bool Test(); }
namespace Test_Addon_Debugger      { bool Test(); }
namespace Test_Addon_WeakRef       { bool Test(); }
namespace Test_Addon_ScriptGrid    { bool Test(); }
namespace Test_Addon_ContextMgr    { bool Test(); }
namespace Test_Addon_ScriptFile    { bool Test(); }
namespace Test_Addon_DateTime      { bool Test(); }
namespace Test_Addon_StdString     { bool Test(); }
namespace Test_Addon_ScriptSocket  { bool Test(); }

#include "utils.h"

// This class should be declared as a global singleton so the leak detection is initiated as soon as possible
#if defined(_MSC_VER)
class MemoryLeakDetector
{
public:
	MemoryLeakDetector()
	{
		_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

		// Use _CrtSetBreakAlloc(n) to find a specific memory leak
		// Remember to "Enable Windows Debug Heap Allocator" in the debug options on MSVC2015. Without it
		// enabled the memory allocation numbers shifts randomly from one execution to another making it
		// impossible to predict the correct number for a specific allocation.
		//_CrtSetBreakAlloc(124);
	}
} g_leakDetector;
#endif

// This class is just to verify that releasing the engine as part of the cleanup
// of global variables doesn't cause crashes due to out-of-order cleanup with
// internally shared objects in the engine.
class EngineDestroyer
{
public:
	EngineDestroyer() { en = 0; }
	~EngineDestroyer() { if( en ) en->Release(); }
	asIScriptEngine *en;
} g_engine;

//----------------------------------
// Test with these flags as well
//
// + AS_MAX_PORTABILITY
//----------------------------------

// Run all tests even if some fail, and list the failed ones at the end
static const int MAX_FAILED_TESTS = 256;
static const char *g_failedTests[MAX_FAILED_TESTS];
static int g_numFailedTests = 0;

#define RUN_TEST(test, name) \
	if( test ) \
	{ \
		PRINTF("-- " name " FAILED\n"); \
		if( g_numFailedTests < MAX_FAILED_TESTS ) g_failedTests[g_numFailedTests++] = name; \
	} \
	else PRINTF("-- " name " passed\n")

int allTests()
{
	PRINTF("AngelScript version: %s\n", asGetLibraryVersion());
	PRINTF("AngelScript options: %s\n", asGetLibraryOptions());

#ifndef AS_MAX_PORTABILITY
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		PRINTF("Compile the tests with AS_MAX_PORTABILITY defined\n");
		goto failed;
	}
#endif

#ifdef __dreamcast__
	fs_chdir(asTestDir);
#endif

	InstallMemoryManager();

	RUN_TEST( Test_Addon_Autowrapper::Test(), "Test_Addon_Autowrapper" );
	if( strstr(asGetLibraryOptions(), "AS_NO_COMPILER")!=0)
		PRINTF("Skipping compiler tests as AS_NO_COMPILER is defined\n");
	else
	{
		RUN_TEST( Test_Addon_ScriptBuilder::Test(), "Test_Addon_ScriptBuilder" );
		RUN_TEST( Test_Addon_ScriptFile::Test(), "Test_Addon_ScriptFile" );
		RUN_TEST( Test_Addon_ContextMgr::Test(), "Test_Addon_ContextMgr" );
		RUN_TEST( Test_Addon_ScriptGrid::Test(), "Test_Addon_ScriptGrid" );
		RUN_TEST( Test_Addon_WeakRef::Test(), "Test_Addon_WeakRef" );
		RUN_TEST( Test_Addon_Debugger::Test(), "Test_Addon_Debugger" );
		RUN_TEST( Test_Addon_ScriptBuilder::Test(), "Test_Addon_ScriptBuilder" );
		RUN_TEST( Test_Addon_ScriptMath::Test(), "Test_Addon_ScriptMath" );
		RUN_TEST( Test_Addon_Serializer::Test(), "Test_Addon_Serializer" );
		RUN_TEST( Test_Addon_ScriptHandle::Test(), "Test_Addon_ScriptHandle" );
		RUN_TEST( Test_Addon_ScriptArray::Test(), "Test_Addon_ScriptArray" );
		RUN_TEST( Test_Addon_Dictionary::Test(), "Test_Addon_Dictionary" );
		RUN_TEST( Test_Addon_DateTime::Test(), "Test_Addon_DateTime" );
		RUN_TEST( Test_Addon_StdString::Test(), "Test_Addon_StdString" );
#ifndef _WIN32
		PRINTF("Skipping test Addon_ScriptSocket as it only works on Windows\n");
#else
		RUN_TEST( Test_Addon_ScriptSocket::Test(), "Test_Addon_ScriptSocket" );
#endif

		RUN_TEST( TestLiteral::Test(), "TestLiteral" );
		RUN_TEST( TestForEach::Test(), "TestForEach" );
		RUN_TEST( TestContext::Test(), "TestContext" );
		RUN_TEST( TestComposition::Test(), "TestComposition" );
		RUN_TEST( TestPropIntegerDivision::Test(), "TestPropIntegerDivision" );
		RUN_TEST( TestThisCallMethod_ConfigErrors::Test(), "TestThisCallMethod_ConfigErrors" );
		RUN_TEST( TestAuto::Test(), "TestAuto" );
		RUN_TEST( TestPow::Test(), "TestPow" );
		RUN_TEST( TestMixin::Test(), "TestMixin" );
		RUN_TEST( TestNamespace::Test(), "TestNamespace" );
		RUN_TEST( TestShared::Test(), "TestShared" );
		RUN_TEST( TestDefaultArg::Test(), "TestDefaultArg" );
		RUN_TEST( TestNamedArgs::Test(), "TestNamedArgs" );
		RUN_TEST( TestScriptRetRef::Test(), "TestScriptRetRef" );
		RUN_TEST( TestGarbageCollect::Test(), "TestGarbageCollect" );
		RUN_TEST( TestFunctionPtr::Test(), "TestFunctionPtr" );
		RUN_TEST( TestModule::Test(), "TestModule" );
		RUN_TEST( TestGetSet::Test(), "TestGetSet" );
		RUN_TEST( TestOperator::Test(), "TestOperator" );
		RUN_TEST( TestTemplate::Test(), "TestTemplate" );
		RUN_TEST( TestDump::Test(), "TestDump" );
		RUN_TEST( TestInheritance::Test(), "TestInheritance" );
		RUN_TEST( TestScriptClassMethod::Test(), "TestScriptClassMethod" );
		RUN_TEST( TestScriptString::Test(), "TestScriptString" );
		RUN_TEST( TestSaveLoad::Test(), "TestSaveLoad" );
		RUN_TEST( TestInterface::Test(), "TestInterface" );
		RUN_TEST( TestCastOp::Test(), "TestCastOp" );
		RUN_TEST( Test2Modules(), "Test2Modules" );
		RUN_TEST( TestArrayObject::Test(), "TestArrayObject" );
		RUN_TEST( TestCompiler::Test(), "TestCompiler" );
		RUN_TEST( TestOptimize(), "TestOptimize" );
		RUN_TEST( TestConversion::Test(), "TestConversion" );
		RUN_TEST( TestRegisterType::Test(), "TestRegisterType" );
		RUN_TEST( TestRefArgument::Test(), "TestRefArgument" );
		RUN_TEST( TestStream::Test(), "TestStream" );
		RUN_TEST( TestEnum::Test(), "TestEnum" );
		RUN_TEST( TestDynamicConfig::Test(), "TestDynamicConfig" );
		RUN_TEST( TestObjHandle::Test(), "TestObjHandle" );
		RUN_TEST( TestGlobalVar(), "TestGlobalVar" );
		RUN_TEST( TestScriptStruct::Test(), "TestScriptStruct" );
		RUN_TEST( TestRZ::Test(), "TestRZ" );
		RUN_TEST( TestArray::Test(), "TestArray" );
		RUN_TEST( TestAny::Test(), "TestAny" );
		RUN_TEST( TestObjHandle2::Test(), "TestObjHandle2" );
		RUN_TEST( TestVector3(), "TestVector3" );
		RUN_TEST( TestConstObject::Test(), "TestConstObject" );
		RUN_TEST( TestImplicitCast::Test(), "TestImplicitCast" );
		RUN_TEST( TestImplicitHandle::Test(), "TestImplicitHandle" );
		RUN_TEST( TestFor::Test(), "TestFor" );
		RUN_TEST( TestRefCast::Test(), "TestRefCast" );
		RUN_TEST( TestStdString(), "TestStdString" );
		RUN_TEST( TestStack2::Test(), "TestStack2" );
		RUN_TEST( TestStdVector::Test(), "TestStdVector" );
		RUN_TEST( TestArrayHandle::Test(), "TestArrayHandle" );
		RUN_TEST( TestDict::Test(), "TestDict" );
		RUN_TEST( TestMultiAssign::Test(), "TestMultiAssign" );
		RUN_TEST( TestException::Test(), "TestException" );
		RUN_TEST( TestInt8::Test(), "TestInt8" );
		RUN_TEST( TestGeneric::Test(), "TestGeneric" );
		RUN_TEST( TestBStr(), "TestBStr" );
		RUN_TEST( TestTypedef::Test(), "TestTypedef" );
		RUN_TEST( TestImport::Test(), "TestImport" );
		RUN_TEST( TestAssign::Test(), "TestAssign" );
		RUN_TEST( TestSwitch(), "TestSwitch" );
		RUN_TEST( TestExceptionMemory::Test(), "TestExceptionMemory" );
		RUN_TEST( TestObject::Test(), "TestObject" );
		RUN_TEST( TestFactory::Test(), "TestFactory" );
		RUN_TEST( TestFuncOverload::Test(), "TestFuncOverload" );
		RUN_TEST( TestObjZeroSize::Test(), "TestObjZeroSize" );
		RUN_TEST( TestSingleton::Test(), "TestSingleton" );
		RUN_TEST( TestCondition(), "TestCondition" );
		RUN_TEST( TestObject2::Test(), "TestObject2" );
		RUN_TEST( TestShark::Test(), "TestShark" );
		RUN_TEST( TestBool::Test(), "TestBool" );
		RUN_TEST( TestBits::Test(), "TestBits" );
		RUN_TEST( TestDestructor::Test(), "TestDestructor" );
		RUN_TEST( TestConstructor2::Test(), "TestConstructor2" );
		RUN_TEST( TestUnsafeRef::Test(), "TestUnsafeRef" );
		RUN_TEST( TestVarType::Test(), "TestVarType" );
		RUN_TEST( TestScriptMath::Test(), "TestScriptMath" );
		RUN_TEST( TestDebug::Test(), "TestDebug" );
		RUN_TEST( TestGetArgPtr::Test(), "TestGetArgPtr" );
		RUN_TEST( TestAutoHandle::Test(), "TestAutoHandle" );
		RUN_TEST( TestObject3::Test(), "TestObject3" );
		RUN_TEST( TestArrayIntf::Test(), "TestArrayIntf" );
		RUN_TEST( TestConstProperty::Test(), "TestConstProperty" );
		RUN_TEST( TestSuspend::Test(), "TestSuspend" );
		RUN_TEST( TestVector3_2::Test(), "TestVector3_2" );
		RUN_TEST( TestNested(), "TestNested" );
		RUN_TEST( TestConstructor::Test(), "TestConstructor" );
		RUN_TEST( TestExecuteScript(), "TestExecuteScript" );
		RUN_TEST( TestCustomMem::Test(), "TestCustomMem" );
		RUN_TEST( TestPostProcess::Test(), "TestPostProcess" );
		RUN_TEST( TestArgRef::Test(), "TestArgRef" );
		RUN_TEST( TestNotInitialized(), "TestNotInitialized" );
		RUN_TEST( TestConfig::Test(), "TestConfig" );
		RUN_TEST( TestInt64(), "TestInt64" );
		RUN_TEST( TestImport2::Test(), "TestImport2" );
		RUN_TEST( TestEnumGlobVar(), "TestEnumGlobVar" );
		RUN_TEST( TestConfigAccess::Test(), "TestConfigAccess" );
		RUN_TEST( TestDiscard::Test(), "TestDiscard" );
		RUN_TEST( TestParser::Test(), "TestParser" );
		RUN_TEST( TestFloat::Test(), "TestFloat" );
		RUN_TEST( TestTempVar(), "TestTempVar" );
		RUN_TEST( TestModuleRef(), "TestModuleRef" );
		RUN_TEST( TestExecuteString(), "TestExecuteString" );
		RUN_TEST( TestStack(), "TestStack" );
		RUN_TEST( TestCreateEngine(), "TestCreateEngine" );
		RUN_TEST( TestLongToken(), "TestLongToken" );
		RUN_TEST( TestOutput::Test(), "TestOutput" );
		RUN_TEST( Test2Func::Test(), "Test2Func" );
		RUN_TEST( TestCircularImport::Test(), "TestCircularImport" );
		RUN_TEST( TestNeverVisited(), "TestNeverVisited" );
		RUN_TEST( TestReturnString::Test(), "TestReturnString" );
		RUN_TEST( TestNegateOperator(), "TestNegateOperator" );

		// The following tests are designed specifically to test the native calling conventions.
		// These are grouped by calling convention and ordered in increasing complexity.
		{
			// cdecl
			RUN_TEST( TestExecute(), "TestExecute" );
			RUN_TEST( TestCDeclReturn::Test(), "TestCDeclReturn" );
			RUN_TEST( TestExecute1Arg(), "TestExecute1Arg" );
			RUN_TEST( TestExecute2Args(), "TestExecute2Args" );
			RUN_TEST( TestExecute4Args(), "TestExecute4Args" );
			RUN_TEST( TestExecute4Argsf(), "TestExecute4Argsf" );
			RUN_TEST( TestExecuteMixedArgs(), "TestExecuteMixedArgs" );
			RUN_TEST( TestExecute32Args(), "TestExecute32Args" );
			RUN_TEST( TestExecute32MixedArgs(), "TestExecute32MixedArgs" );
			RUN_TEST( TestCDecl_Class(), "TestCDecl_Class" );
			RUN_TEST( TestCDecl_ClassA(), "TestCDecl_ClassA" );
			RUN_TEST( TestCDecl_ClassC(), "TestCDecl_ClassC" );
			RUN_TEST( TestCDecl_ClassD(), "TestCDecl_ClassD" );
			RUN_TEST( TestCDecl_ClassK(), "TestCDecl_ClassK" );

			// cdecl_objlast and cdecl_objfirst
			RUN_TEST( TestCDeclObjLast::Test(), "TestCDeclObjLast" );
			RUN_TEST( TestReturnWithCDeclObjFirst(), "TestReturnWithCDeclObjFirst" );

			// thiscall
			RUN_TEST( TestExecuteThis32MixedArgs(), "TestExecuteThis32MixedArgs" );
			RUN_TEST( TestThiscallClass(), "TestThiscallClass" );
			RUN_TEST( TestNotComplexThisCall(), "TestNotComplexThisCall" );
			RUN_TEST( TestVirtualMethod(), "TestVirtualMethod" );
			RUN_TEST( TestMultipleInheritance(), "TestMultipleInheritance" );
			RUN_TEST( TestVirtualInheritance(), "TestVirtualInheritance" );

			// thiscall_asglobal
			RUN_TEST( TestThiscallAsGlobal::Test(), "TestThiscallAsGlobal" );

			// thiscall_objlast & objfirst
			RUN_TEST( TestThisCallMethod::Test(), "TestThisCallMethod" );

			// stdcall
			RUN_TEST( TestStdcall4Args(), "TestStdcall4Args" );
			RUN_TEST( TestNotComplexStdcall(), "TestNotComplexStdcall" );

			// Default functions
			RUN_TEST( Test_Native_DefaultFunc::Test(), "Test_Native_DefaultFunc" );
		}
	}

	// This test uses ATL::CString thus it is turned off by default
//	RUN_TEST( TestCString::Test(), "TestCString" );
	// Pointers are not supported by AngelScript at the moment, but they may be in the future
//	RUN_TEST( TestPointer::Test(), "TestPointer" );

	RemoveMemoryManager();

	// Populate the global engine destroyer after the memory manager has
	// been removed so we don't get false positives for memory leaks.
	g_engine.en = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	if( g_numFailedTests )
	{
		PRINTF("--------------------------------------------\n");
		PRINTF("%d test(s) failed:\n", g_numFailedTests);
		for( int n = 0; n < g_numFailedTests; n++ )
			PRINTF("   %s\n", g_failedTests[n]);
		goto failed;
	}

//succeed:
	PRINTF("--------------------------------------------\n");
	PRINTF("All of the tests passed with success.\n\n");
#if defined(WAIT_ON_END) && !defined(_M_ARM) && (defined(WIN32) || defined(_WIN64))
	PRINTF("Press any key to quit.\n");
	while(!_getch());
#endif
	return 0;

failed:
	PRINTF("--------------------------------------------\n");
	PRINTF("One of the tests failed, see details above.\n\n");
#if defined(WAIT_ON_END) && !defined(_M_ARM) && (defined(WIN32) || defined(_WIN64))
	PRINTF("Press any key to quit.\n");
	while(!_getch());
#endif
	return -1;
}

#if !defined(IS_IOS) && !defined(IS_ANDROID)
// ios and android has their entry points.
// For iOS call allTests() from application entry point.
int main(int /* argc */, char ** /* argv */)
{
	return allTests();
}
#endif

#ifdef IS_ANDROID
// Android entry function (called from java of android test project)
#include <jni.h>

extern "C" {
	void Java_com_angelcode_android_AndroidTests_runTests( JNIEnv* env, jobject thiz )
	{
		allTests();
	}
}
#endif

