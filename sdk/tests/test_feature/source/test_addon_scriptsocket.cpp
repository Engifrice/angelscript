#include "utils.h"
#include "../../../add_on/scriptsocket/scriptsocket.h"
#include "../../../add_on/scriptstdstring/scriptstdstring.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
#include "../../../add_on/contextmgr/contextmgr.h"
#include "../../../add_on/autowrapper/aswrappedcall.h"
#include <chrono>

namespace Test_Addon_ScriptSocket
{

std::string output;
void print(const std::string& str)
{
	output += str;
//	PRINTF("%s", str.c_str());
}

bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	asIScriptEngine* engine;
	int r;

	// TODO: Test trying to listen to a port that is already occupied
	// TODO: Test that it is possible to successfully detect if the remove socket is closed/lost

	// Test setting up a listener socket and then connecting to it to both send and receive
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		bout.buffer = "";

		RegisterScriptArray(engine, false);
		RegisterStdString(engine);
		RegisterScriptDictionary(engine);

		CContextMgr ctxMgr;
		ctxMgr.RegisterCoRoutineSupport(engine);

		RegisterScriptSocket(engine);
#ifdef AS_MAX_PORTABILITY
		engine->RegisterGlobalFunction("void print(const string &in)", WRAP_FN(print), asCALL_GENERIC);
#else
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);
#endif

		bool success = false;
		engine->RegisterGlobalProperty("bool success", &success);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", R"script(
			void main()
			{
				socket a;
				a.listen(39000);                        // Start listener socket
				print("Server: Listening on 39000...\n");
				createCoRoutine(client, null);          // Create the client co-routine
				bool sent = false;
				string msg;
				socket @b;
				for(;;)
				{
					if( b is null )
					{
						@b = a.accept();
						if( b !is null )
							print("Server: Identified a client\n");
					}
					if ( b !is null )
					{
						if( !sent )
						{
							print("Server: Sending 'hello'\n");
							b.send('hello');            // Send hello to the client
							sent = true;
						}
						else
						{
							string pkg = b.receive(); // Receive the response from the client
							if( pkg.length() > 0 )
								print("Server: Received '" + pkg + "'\n");
							msg += pkg;
							if( msg == 'to you too' )
							{
								print("Server: Completed message '" + msg + "'\n");
								success = true;
								break;
							}
						}
					}
					yield();
				}
			}
			void client(dictionary @args)
			{
				socket a;
				print("Client: Connecting to 39000\n");
				a.connect(0x7F000001,39000); // connect to local host 127.0.0.1, port 39000
				print("Client: Connected\n");
				bool first = true;
				for(;;)
				{
					if( first )
					{
						string msg = a.receive();
						if( msg.length() > 0 )
							print("Client: Received '" + msg + "'\n");
						if( msg == 'hello' )
						{
							print("Client: Sending 'to you'\n");
							a.send('to you'); // send the first response to the server
							print("Client: Sent\n");
							first = false;
						}
					}
					else
					{
						print("Client: Sending ' too'\n");
						a.send(' too'); // send the second package
						print("Client: Sent\n");
						break;
					}
					yield();
				}
			}
			)script");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		// Run the script
		ctxMgr.AddContext(engine, mod->GetFunctionByName("main"));
		// The number of iterations needed depends on the timing of the socket, which varies with the
		// speed of the build and the machine, so the scripts are given a time limit rather than a
		// fixed number of iterations. The scripts must still need several iterations, as they yield
		int iterations = 0;
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		while (ctxMgr.ExecuteScripts() > 0 && std::chrono::steady_clock::now() - start < std::chrono::seconds(10))
			iterations++;

		if (iterations < 6)
		{
			PRINTF("iterations = %d\n", iterations);
			TEST_FAILED;
		}

		ctxMgr.AbortAll();

		engine->ShutDownAndRelease();

		if (output !=
			"Server: Listening on 39000...\n"
			"Client: Connecting to 39000\n"
			"Client: Connected\n"
			"Server: Identified a client\n"
			"Server: Sending 'hello'\n"
			"Client: Received 'hello'\n"
			"Client: Sending 'to you'\n"
			"Client: Sent\n"
			"Server: Received 'to you'\n"
			"Client: Sending ' too'\n"
			"Client: Sent\n"
			"Server: Received ' too'\n"
			"Server: Completed message 'to you too'\n")
		{
			PRINTF("%s", output.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	return fail;
}

}
