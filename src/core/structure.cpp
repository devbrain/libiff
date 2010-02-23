#include "core/structure.hpp"

namespace iff 
{
  object_c::~object_c ()
  {
  }
  // =========================================================
  chunk_c::chunk_c (const std::string& tag, std::streamsize offset, std::streamsize size)
    : m_id     (tag),
      m_offset (offset),
      m_size   (size)
  {
  }
  // --------------------------------------------------------
  bool chunk_c::is_group () const
  {
    return false;
  }
  // --------------------------------------------------------
  std::streamsize chunk_c::offset () const
  {
    return m_offset;
  }
  // --------------------------------------------------------
  std::streamsize chunk_c::size () const
  {
    return m_size;
  }
  // --------------------------------------------------------
  std::string chunk_c::id () const
  {
    return m_id;
  }
  // =========================================================
  group_c::group_c (const std::string& tag, std::streamsize offset, std::streamsize size)
    : m_id     (tag),
      m_sub_id (tag),
      m_offset (offset),
      m_size   (size)
  {
  }
  // -----------------------------------------------------------
  group_c::group_c (const std::string& tag, const std::string& sub_tag, 
		    std::streamsize offset, std::streamsize size)
    : m_id     (tag),
      m_sub_id (sub_tag),
      m_offset (offset),
      m_size   (size)
  {
  }
  // -----------------------------------------------------------
  group_c::~group_c ()
  {
    for (objects_t::iterator i = m_objects.begin (); i != m_objects.end (); i++)
      {
	object_c* victim = *i;
	delete victim;
      }
    m_objects.clear ();
  }
  // -----------------------------------------------------------
  bool group_c::is_group  () const
  {
    return true;
  }
  // -----------------------------------------------------------
  std::streamsize group_c::offset () const
  {
    return m_offset;
  }
  // -----------------------------------------------------------
  std::streamsize group_c::size () const
  {
    return m_size;
  }
  // -----------------------------------------------------------
  std::string group_c::id () const
  {
    return m_id;
  }
  // -----------------------------------------------------------
  std::string group_c::sub_id () const
  {
    return m_sub_id;
  }
  // -----------------------------------------------------------
  group_c::iterator_t group_c::begin () const
  {
    return m_objects.begin ();
  }
  // -----------------------------------------------------------
  group_c::iterator_t group_c::end   () const
  {
    return m_objects.end ();
  }
  // -----------------------------------------------------------
  void group_c::add (object_c* obj)
  {
    m_objects.push_back (obj);
  }
  // ===========================================================
  structure_c::structure_c (const char* file_name, std::streamsize file_size)
    : m_file_name (file_name),
      m_root      ("", 0, file_size)
  {
  }
  // -----------------------------------------------------------
  structure_c::~structure_c ()
  {
  }
  // -----------------------------------------------------------
  void structure_c::add (object_c* obj)
  {
    m_root.add (obj);
  }
  // -----------------------------------------------------------
  structure_c::iterator_t structure_c::begin () const
  {
    return m_root.begin ();
  }
  // -----------------------------------------------------------
  structure_c::iterator_t structure_c::end   () const
  {
    return m_root.end ();
  }
  // -----------------------------------------------------------
  std::string structure_c::file_name () const
  {
    return m_file_name;
  }
  // -----------------------------------------------------------
  std::streamsize structure_c::file_size () const
  {
    return m_root.size ();
  }
}
