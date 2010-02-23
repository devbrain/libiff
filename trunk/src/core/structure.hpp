#ifndef __IFF_STRUCTURE_HPP__
#define __IFF_STRUCTURE_HPP__

#include <string>
#include <iostream>
#include <list>

namespace iff
{
  class object_c
  {
  public:
    object_c ();
    virtual ~object_c ();
    
    virtual bool            is_group  () const = 0;
    virtual std::streamsize offset    () const = 0;
    virtual std::streamsize size      () const = 0;
    virtual std::string     id        () const = 0;
  };
  // ====================================================================================
  class chunk_c : public object_c
  {
  public:
    chunk_c (const std::string& tag, std::streamsize offset, std::streamsize size);
    
    virtual bool            is_group  () const;
    virtual std::streamsize offset    () const;
    virtual std::streamsize size      () const;
    virtual std::string     id        () const;
  private:
    const std::string     m_id;
    const std::streamsize m_offset;
    const std::streamsize m_size;
  };
  // ====================================================================================
  class group_c : public object_c
  {
  public:
    typedef std::list <object_c*>     objects_t;
    typedef objects_t::const_iterator iterator_t;
  public:
    group_c (const std::string& tag, std::streamsize offset, std::streamsize size);
    group_c (const std::string& tag, const std::string& sub_tag, 
	     std::streamsize offset, std::streamsize size);

    virtual ~group_c ();
    virtual bool            is_group  () const;
    virtual std::streamsize offset    () const;
    virtual std::streamsize size      () const;
    virtual std::string     id        () const;
    std::string sub_id () const;
    iterator_t begin () const;
    iterator_t end   () const;

    void add (object_c* obj);
  private:
    const std::string     m_id;
    const std::string     m_sub_id;
    const std::streamsize m_offset;
    const std::streamsize m_size;
    objects_t             m_objects;
  };
  // ====================================================================================
  class structure_c
  {
    typedef group_c::iterator_t iterator_t;
  public:
    structure_c (const char* file_name, std::streamsize file_size);
    ~structure_c ();

    void add (object_c* obj);
    
    iterator_t begin () const;
    iterator_t end   () const;

    std::string file_name () const;
    std::streamsize file_size () const;
  private:
    std::string     m_file_name;
    group_c         m_root;
  };
  
} // ns iff


#endif
