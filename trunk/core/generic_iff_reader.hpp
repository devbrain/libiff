#ifndef __GENERIC_IFF_READER_HPP__
#define __GENERIC_IFF_READER_HPP__

#include <fstream>

enum status_t
  {
    eOK,
    eNOT_IFF,
    eIO_ERROR
  };


template <class IO_POLICY>
class generic_iff_reader_c
{
public:
  typedef typename IO_POLICY::word_t word_t;
  typedef typename IO_POLICY::id_t   id_t;
  
public:
  generic_iff_reader_c ();
  virtual ~generic_iff_reader_c ();
  status_t open (const char* path);
  status_t read ();
protected:
  // CALLBACKS
  virtual void _on_chunk_enter (const id_t& id, std::streamsize chunk_size, std::streamsize file_pos) = 0;

  virtual void _on_chunk_exit  (const id_t& id, std::streamsize chunk_size, std::streamsize file_pos) = 0;

  virtual void _on_group_enter (const id_t& id, const id_t& tag, 
				std::streamsize group_size, std::streamsize file_pos) = 0;
  
  virtual void _on_group_exit  (const id_t& id, const id_t& tag,
				std::streamsize group_size, std::streamsize file_pos) = 0;
private:
  status_t _on_chunk_enter_safe (const id_t& id, std::streamsize chunk_size, std::streamsize file_pos);

  status_t _on_chunk_exit_safe  (const id_t& id, std::streamsize chunk_size, std::streamsize file_pos);

  status_t _on_group_enter_safe (const id_t& id, const id_t& tag, 
				 std::streamsize group_size, 
				 std::streamsize file_pos);
  
  status_t _on_group_exit_safe  (const id_t& id, const id_t& tag,
				 std::streamsize group_size, 
				 std::streamsize file_pos);
private:
  status_t _read_group (const id_t& id, std::streamsize& has_so_far);
  status_t _read_group_contents (std::streamsize group_size, std::streamsize& has_so_far);
  status_t _read_chunk (const id_t& id, std::streamsize& has_so_far);
private:
  std::ifstream   m_ifs;
  std::streamsize m_file_size;
};

// ===================================================================
template <class IO_POLICY>
generic_iff_reader_c<IO_POLICY>::generic_iff_reader_c ()
{
}
// -------------------------------------------------------------------
template <class IO_POLICY>
generic_iff_reader_c<IO_POLICY>::~generic_iff_reader_c ()
{
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::open (const char* path)
{
  m_ifs.open (path, std::ios::binary);
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  m_ifs.seekg (0, std::ios::end);
  m_file_size = m_ifs.tellg();
  m_ifs.seekg (0, std::ios::beg);
  if (IO_POLICY::has_header ())
    {
      const unsigned w = IO_POLICY::words_in_header ();
      word_t* hdr = new word_t [w];
      for (unsigned i = 0; i<w; i++)
	{
	  hdr [i] = IO_POLICY::read (m_ifs);
	}
      const bool rc = IO_POLICY::check_header (hdr);
      delete [] hdr;
      if (!rc)
	{
	  return eNOT_IFF;
	}
    }
  return eOK;
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::read ()
{
  word_t w = IO_POLICY::read (m_ifs);
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  
  id_t id (w);
  std::streamsize has_so_far = sizeof (w);
  if (IO_POLICY::is_group (id))
    {
      return _read_group (id, has_so_far);
    }
  if (!IO_POLICY::should_start_with_group ())
    {
      return _read_chunk (id, has_so_far);
    }
  return eNOT_IFF;
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_read_group (const id_t& id, std::streamsize& has_so_far)
{
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  const word_t group_size               = IO_POLICY::read (m_ifs);

  has_so_far += sizeof (group_size);
  const std::streamsize real_group_size = IO_POLICY::real_size (group_size);
  const std::streamsize group_start     = m_ifs.tellg();
  id_t tag = id;
  std::streamsize in_grp_size = 0;
  if (IO_POLICY::group_has_tag ())
    {
      word_t w = IO_POLICY::read (m_ifs);
      has_so_far += sizeof (word_t);
      in_grp_size += sizeof (word_t);
      if (!m_ifs.good ())
	{
	  return eIO_ERROR;
	}
      tag = id_t (w);
    }
  
  if (eOK != this->_on_group_enter_safe (id, tag, group_size, group_start))
    {
      return eIO_ERROR;
    }
  
  status_t rc = _read_group_contents (group_size, in_grp_size);
  if (rc != eOK)
    {
      return rc;
    }
  has_so_far += in_grp_size;


  m_ifs.seekg (group_start, std::ios::beg);
  m_ifs.seekg (real_group_size, std::ios::cur);
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  
  if (eOK != this->_on_group_exit_safe (id, tag, group_size, group_start + real_group_size))
    {
      return eIO_ERROR;
    }
  return eOK;
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_read_chunk (const id_t& id, std::streamsize& has_so_far)
{
  const word_t chunk_size = IO_POLICY::read (m_ifs);
  has_so_far += sizeof (word_t);
  const std::streamsize now = m_ifs.tellg();
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  if (eOK != this->_on_chunk_enter_safe (id, chunk_size, now))
    {
      return eIO_ERROR;
    }
  const std::streamsize skip = IO_POLICY::real_size (chunk_size);
  m_ifs.seekg (skip, std::ios::cur);
  if (!m_ifs.good ())
    {
      return eIO_ERROR;
    }
  has_so_far += skip;
  return this->_on_chunk_exit_safe (id, chunk_size, now + skip);
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_read_group_contents (std::streamsize group_size, std::streamsize& has_so_far)
{
  
  while (has_so_far < group_size)
    {
      const std::streamsize start = m_ifs.tellg ();

      word_t w = IO_POLICY::read (m_ifs);
      

      id_t id (w);
      if (IO_POLICY::is_group (id))
	{
	  std::streamsize gs = sizeof (word_t);
	  status_t rc =  _read_group (id, gs);
	  if (rc != eOK)
	    {
	      return rc;
	    }
	  has_so_far += gs;
	}
      else
	{
	  has_so_far += sizeof (w);
	  std::streamsize cs = 0;
	  status_t rc = _read_chunk (id, cs);
	  if (rc != eOK)
	    {
	      return rc;
	    }
	  has_so_far += cs;
	}
    }
  return eOK;
}
// -------------------------------------------------------------------
#define IFF_SAFE_PROLOG							\
  const std::streamsize now = m_ifs.tellg();				\
  if (!m_ifs.good ())							\
    return eIO_ERROR

#define IFF_SAFE_EPILOG							\
  if (!m_ifs.good ()) return eIO_ERROR;					\
  m_ifs.seekg (now, std::ios::beg);					\
  if (!m_ifs.good ())  return eIO_ERROR;				\
  return eOK
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_on_chunk_enter_safe (const id_t& id, 
						       std::streamsize chunk_size, 
						       std::streamsize file_pos)
{
  IFF_SAFE_PROLOG;
  this->_on_chunk_enter (id, chunk_size, file_pos);
  IFF_SAFE_EPILOG;
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_on_chunk_exit_safe  (const id_t& id, std::streamsize chunk_size, 
						       std::streamsize file_pos)
{
  IFF_SAFE_PROLOG;
  this->_on_chunk_exit (id, chunk_size, file_pos);
  IFF_SAFE_EPILOG;
}

// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_on_group_enter_safe (const id_t& id, const id_t& tag, 
						       std::streamsize chunk_size, 
						       std::streamsize file_pos)
{
  IFF_SAFE_PROLOG;
  this->_on_group_enter (id, tag, chunk_size, file_pos);
  IFF_SAFE_EPILOG;
}
// -------------------------------------------------------------------
template <class IO_POLICY>
status_t
generic_iff_reader_c<IO_POLICY>::_on_group_exit_safe  (const id_t& id, const id_t& tag,
						       std::streamsize chunk_size, 
						       std::streamsize file_pos)
{
  IFF_SAFE_PROLOG;
  this->_on_group_exit (id, tag, chunk_size, file_pos);
  IFF_SAFE_EPILOG;
}
#endif

