#include <iostream>
#include <cassert>
#include <cstring>
#include "msp.h"

using namespace std;

int main()
{
  Msp msp;

  std::string s;
  file_handler * zero = msp["/dev/zero"];
  zero->read(s, 5);
  //assert( s.length() == 5 );
  //assert( ! memcmp (s.c_str(), "\0\0\0\0\0", 5) );
  
  msp.create_file("/tmp.txt");
  msp["/tmp.txt"]->write("lorem ipsum");
  msp["/tmp.txt"]->read(s, 5);
  assert(s == "lorem");
  msp["/tmp.txt"]->read(s);
  assert(s == "lorem ipsum");
  
  msp.copy("/tmp.txt", "/tmp.txt.copy");
  msp.symlink("/tmp.txt","/tmp.txt.link");
  msp["/tmp.txt.copy"]->read(s);
  assert(s == "lorem ipsum");
  msp["/tmp.txt"]->write("dolor sit amet");
  msp["/tmp.txt.copy"]->read(s);
  assert(s == "lorem ipsum");
  msp["/tmp.txt.link"]->read(s);
  assert(s == "dolor sit amet");

  msp.create_desc_dir("/kolejki");
  msp["/kolejki"]->write("katalog z kolejkami");
  
  msp.create_fifo("/kolejki/fifo");
  msp["/kolejki/fifo"]->write("lorem ipsum");
  msp["/kolejki/fifo"]->read(s, 5);
  assert(s == "lorem");
  msp["/kolejki/fifo"]->read(s);
  assert(s == " ipsum");
  
  msp.create_lifo("/kolejki/lifo");

  msp.hardlink("/kolejki/lifo", "/lifo.hardlink");

  msp.symlink("/kolejki/lifo", "/lifo.symlink");

  msp.move("/kolejki", "/nowe.kolejki");
  msp.copy("/nowe.kolejki", "/kopia.kolejek");
  msp["/nowe.kolejki/lifo"]->write("lorem ipsum");
  msp["/lifo.symlink"]->read(s, 5);
  assert(s == "muspi");

  msp.remove("/nowe.kolejki");

  msp["/lifo.hardlink"]->read(s);
  assert(s == " merol");
  
  msp.remove("/tmp.txt");
    
  dir_handler * root = dynamic_cast<dir_handler*> (msp["/"]);
  for (dir_handler::iterator i = root->begin(); i != root->end(); ++i)
	std::cout << i->get_name() << std::endl;
  
// Powyższy kod powinien wypisać:
// dev
// kopia.kolejek
// lifo.hardlink
// tmp.txt.copy

  cout << "-----------" << endl;

// Zaś poniższy kod:
// dev
// null
// zero
// kopia.kolejek
// fifo
// lifo
// lifo.hardlink
// tmp.txt.copy

  // dir_handler * root = dynamic_cast<dir_handler*>(msp["/"]);
  for (dir_handler::rec_iterator i = root->rec_begin(); 
	   i != root->rec_end(); 
	   ++i)
	cout << i->get_name() << endl;

  return 0;
}

