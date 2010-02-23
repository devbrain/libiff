#ifndef __IFF_CORE_PARSER_HPP__
#define __IFF_CORE_PARSER_HPP__

#include <iostream>
#include <string>

namespace iff
{
  
  class parser_c
  {
  public:
    enum status_t
      {
	eOK,
	eBAD_FILE,
	eIO_ERROR,
	eNOT_INIT
      };
  public:
    parser_c ();
    virtual ~parser_c ();
    
    virtual status_t open (const char* filename) = 0;
    virtual status_t read () = 0;
  protected:
    virtual void _on_chunk_enter (const std::string& id, 
				  std::streamsize chunk_size, 
				  std::streamsize file_pos) = 0;

    virtual void _on_chunk_exit  (const std::string& id, 
				  std::streamsize chunk_size, 
				  std::streamsize file_pos) = 0;
    
    virtual void _on_group_enter (const std::string& id, const std::string& tag, 
				  std::streamsize chunk_size, 
				  std::streamsize file_pos) = 0;
    
    virtual void _on_group_exit  (const std::string& id, const std::string& tag,
				  std::streamsize chunk_size, 
				  std::streamsize file_pos) = 0;
  };
}


#endif
