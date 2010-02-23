#include <iostream>
#include "core/generic_parser.hpp"
#include "core/ea/ea_io.hpp"

class ea_iff_reader_c : public iff::generic_parser_c <iff::ea::io_c>
{
public:
  ea_iff_reader_c ()
    : m_level (0)
  {
  }
private:
  
  virtual void _on_chunk_enter (const std::string& id, 
				std::streamsize chunk_size, 
				std::streamsize file_pos);

  virtual void _on_chunk_exit  (const std::string& id, 
				std::streamsize chunk_size, 
				std::streamsize file_pos);

  virtual void _on_group_enter (const std::string& id, const std::string& tag, 
				std::streamsize chunk_size, 
				std::streamsize file_pos);
  
  virtual void _on_group_exit  (const std::string& id, const std::string& tag,
				std::streamsize chunk_size, 
				std::streamsize file_pos);
private:
  int m_level;
};

static std::string ident (int steps)
{
  return  std::string ( (size_t)steps, char (' '));
}
// ---------------------------------------------------------------
void ea_iff_reader_c::_on_chunk_enter (const std::string& id, std::streamsize chunk_size, std::streamsize file_pos)
{
  std::cout << ident (m_level) << " CHUNK: " << id << " : " << chunk_size << " (" << file_pos << ", ";
}
// ---------------------------------------------------------------
void ea_iff_reader_c::_on_chunk_exit  (const std::string& id, std::streamsize chunk_size, std::streamsize file_pos)
{
  std::cout << file_pos << ")" << std::endl;
}
// ---------------------------------------------------------------
void ea_iff_reader_c::_on_group_enter (const std::string& id, const std::string& tag, 
				       std::streamsize group_size, std::streamsize file_pos)
{
  std::cout << ident (m_level++) << "-> GROUP: " << id << "," << tag 
	    << " (" << file_pos << "," << group_size << ")"
	    << std::endl;
}
// ---------------------------------------------------------------
void ea_iff_reader_c::_on_group_exit  (const std::string& id, const std::string& tag,
				       std::streamsize group_size, std::streamsize file_pos)
{
  std::cout << ident (--m_level) << "<- GROUP: " << id << "," << tag 
	    << " (" << file_pos << "," << group_size << ")"
	    << std::endl;
}
// ---------------------------------------------------------------
int main (int argc, char* argv [])
{
  if (argc != 2)
    {
      std::cerr << "USAGE " << argv [0] << " <filename>" << std::endl;
      return 1;
    }
  ea_iff_reader_c ifr;
  const char* ifname = argv[1];
  if (ifr.open (ifname) != iff::parser_c::eOK)
    {
      std::cerr << "cant open "<< std::endl;
      return 1;
    }
  
  if (ifr.read () != iff::parser_c::eOK)
    {
      std::cerr << "cant read "<< std::endl;
      return 1;
    }
  return 0;
}
