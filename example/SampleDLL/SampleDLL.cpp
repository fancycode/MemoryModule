#include "SampleDLL.h"
#include <windows.h>


#include <string>
#include <sstream>
#include <unordered_map>
using namespace std;


// adding this class to track instantiations
class FunnBall {
public:
    FunnBall(const char* ident)
        : _ident(ident){
        ostringstream msg;
        msg << "Hi, I'm in the constructor for FB ident: " << _ident << endl;
        OutputDebugString(msg.str().c_str());
    }
    const string& ident() const { return _ident;  }
private:
    string _ident;
};

// module static, this always seems to work.
// they end up happening in the c++ runtime as part of DllMain
static FunnBall fb1("This is the module static");

// make a function that has static objects inside it
static void havefun() {

    // these guys are required to be initialized before first use by c++
    // They fail to properly initialize with MemoryLoader
    // Looking (with very rusty eyes) at the assembler, it appears that VC
    // is trying to do this trick by writing a conditional so that it can test
    // if it is the first-time through the function, and then doing the initialization
    // only once.  It works with LoadLibrary, but not MemoryLoader.
    // the thing being examined (if you look at the assembler) is a location that
    // is initialized with DD dup(?), which makes no immediate sense.  So I'm guessing
    // that something else in the load process is initializing this DWORD in the LoadLibrary
    // case.
    static FunnBall fb2("This is the static in the function");
    static unordered_map<string, string> damap = {
        {"one", "wun"},
        {"two", "too"}
    };

    ostringstream msg;
    msg << "idents in play: fb1=" << fb1.ident() << ", fb2=" << fb2.ident()
        << ", map has " << damap.size() << endl;
    OutputDebugString(msg.str().c_str());

    if (damap.size() != 2) {
        OutputDebugString("============== FAIL ==============\n");
    }
}


extern "C" {

SAMPLEDLL_API int addNumbers(int a, int b)
{
    // force our test function to run.
    havefun();
    return a + b;
}

}
