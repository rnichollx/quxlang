#include "rylang/compiler.hpp"
#include <iostream>

int main(int argc, char ** argv)
{
  rylang::compiler c(argc, argv);

  auto files = c.get_file_list();

  auto file_name = files.at(0);

  auto file_contents = c.get_file_contents(file_name);

  std::cout << "File contents of " << file_name << ":\n" << file_contents << "\n";


}