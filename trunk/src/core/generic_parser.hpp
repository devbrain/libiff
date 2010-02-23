#ifndef __IFF_GENERIC_PARSER_HPP__
#define __IFF_GENERIC_PARSER_HPP__

#include "core/generic_iff_reader.hpp"
#include "core/parser.hpp"

namespace iff
{
  template <class IO_POLICY> 
  class generic_parser_c : public parser_c
  {
  public:
    generic_parser_c ();
    virtual ~generic_parser_c ();

    virtual status_t open (const char* filename);
    virtual status_t read ();
  private:
    class iff_reader_c : public generic_iff_reader_c <IO_POLICY>
    {
      typedef typename generic_iff_reader_c <IO_POLICY>::id_t id_t;
    public:
      iff_reader_c (generic_parser_c <IO_POLICY>* owner);
    private:
      virtual void _on_chunk_enter (const id_t& id, 
				    std::streamsize chunk_size, 
				    std::streamsize file_pos);

      virtual void _on_chunk_exit  (const id_t& id, 
				    std::streamsize chunk_size, 
				    std::streamsize file_pos);

      virtual void _on_group_enter (const id_t& id, const id_t& tag, 
				    std::streamsize chunk_size, 
				    std::streamsize file_pos);
  
      virtual void _on_group_exit  (const id_t& id, const id_t& tag,
				    std::streamsize chunk_size, 
				    std::streamsize file_pos);
    private:
      generic_parser_c <IO_POLICY>* m_owner;
    };
  private:
    friend class iff_reader_c;

    iff_reader_c* m_reader;
  };
}

// ===================================================
// Implementation
// ===================================================

namespace iff
{
  template <class IO_POLICY> 
  generic_parser_c <IO_POLICY>::generic_parser_c ()
    : m_reader (0)
  {

  }
  // -------------------------------------------------
  template <class IO_POLICY> 
  generic_parser_c <IO_POLICY>::~generic_parser_c ()
  {
    if (m_reader)
      {
	delete m_reader;
      }
  }
  // -------------------------------------------------
  template <class IO_POLICY> 
  typename generic_parser_c <IO_POLICY>::status_t
  generic_parser_c <IO_POLICY>::open (const char* filename)
  {
    if (!m_reader)
      {
	m_reader = new iff_reader_c (this);
      }
    typename generic_iff_reader_c <IO_POLICY>::status_t rc;
    rc = m_reader->open (filename);
    if (rc == generic_iff_reader_c <IO_POLICY>::eOK)
      {
	return eOK;
      }
    if (rc == generic_iff_reader_c <IO_POLICY>::eIO_ERROR)
      {
	return eIO_ERROR;
      }
    return eBAD_FILE;
  }
  // -------------------------------------------------------
  template <class IO_POLICY> 
  typename generic_parser_c <IO_POLICY>::status_t
  generic_parser_c <IO_POLICY>::read ()
  {
    if (!m_reader)
      {
	return eNOT_INIT;
      }
    typename generic_iff_reader_c <IO_POLICY>::status_t rc;
    rc = m_reader->read ();
    if (rc == generic_iff_reader_c <IO_POLICY>::eOK)
      {
	return eOK;
      }
    if (rc == generic_iff_reader_c <IO_POLICY>::eIO_ERROR)
      {
	return eIO_ERROR;
      }
    return eBAD_FILE;
  }
  // =======================================================
  template <class IO_POLICY> 
  generic_parser_c <IO_POLICY>::
  iff_reader_c::iff_reader_c (generic_parser_c <IO_POLICY>* owner)
    : m_owner (owner)
  {
  }
  // ------------------------------------------------------
  template <class IO_POLICY> 
  void 
  generic_parser_c <IO_POLICY>::
  iff_reader_c::_on_chunk_enter (const id_t& id, 
				 std::streamsize chunk_size, 
				 std::streamsize file_pos)
  {
    m_owner->_on_chunk_enter (id.to_string (), chunk_size, file_pos);
  }
  // ------------------------------------------------------
  template <class IO_POLICY> 
  void 
  generic_parser_c <IO_POLICY>::
  iff_reader_c::_on_chunk_exit  (const id_t& id, 
				 std::streamsize chunk_size, 
				 std::streamsize file_pos)
  {
    m_owner->_on_chunk_exit (id.to_string (), chunk_size, file_pos);
  }
  // ------------------------------------------------------
  template <class IO_POLICY> 
  void 
  generic_parser_c <IO_POLICY>::
  iff_reader_c::_on_group_enter (const id_t& id, const id_t& tag, 
				 std::streamsize chunk_size, 
				 std::streamsize file_pos)
  {
    m_owner->_on_group_enter (id.to_string (), 
			      tag.to_string (),
			      chunk_size, 
			      file_pos);
  }
  // ------------------------------------------------------
  template <class IO_POLICY> 
  void 
  generic_parser_c <IO_POLICY>::
  iff_reader_c::_on_group_exit  (const id_t& id, const id_t& tag,
				 std::streamsize chunk_size, 
				 std::streamsize file_pos)
  {
    m_owner->_on_group_exit (id.to_string (), 
			     tag.to_string (),
			     chunk_size, 
			     file_pos);
  }
}// ns iff


#endif
