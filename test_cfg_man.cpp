#include <iostream>
#include "config_manager.h"
#include "my_cfg.h"


using namespace std;



/* 
 TBD: create tests that take a series of commands as input.
 They verify the working of the module by checking that the
 output of a prt command is as expected.

 TBD: create tests that can verify that data is correctly saved
 to NVRAM.
*/


int main()
{
    config_manager * cm = new config_manager(pBaseDesc);
    
    while (true)
    {        
        string cmd;
        char * ex;

        cout << ">";

        cin >> cmd;

        cout << cmd;
      

        ex = (char *)cmd.c_str();
        
        cm->do_cmd(1, &ex);
    }
    
}




