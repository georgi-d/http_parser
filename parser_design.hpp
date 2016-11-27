/// HTTP parser

template <bool IsRequest, bool ChunkDecodeBody = true>
class parser_v1 {

   /*
    Parses an HTTP message filling the fields and the body buffer

    @param buffer [in] raw http buffer to read from
    @param fields [in/out] container to add the headers, trailers and verb/status
    @param body [out] buffer to store any parsed body data
    @param ec [out] error::need_more if more input is needed to complete the message
                    error::need_more && result.first = 0 if more input is needed while preserving the existing one to complete the message
                    error::have_more if the body buffer is full and there is more data in the input
                    parse error 

    @return pair containing bytes_consumed and body_bytes
   */
   template <Fields>
   pair<size_t, size_t>
   write(const asio::mutable_buffer& buffer, Fields& fields, asio::mutable_buffer& body, error_code& ec);


   /*
    Parses an HTTP message headers only flling the fields

    @param buffer [in] raw http buffer to read from
    @param fields [in/out] container to add the headers, trailers and verb/status
    @param body [out] buffer to store any parsed body data
    @param ec [out] error code if error is encountered or error::need_more && result.first = 0 if the end of the headers have not been reached

    @return pair containing bytes_consumed and body_bytes
   */
   template <class ConstBufferSequence, class Fields>
   pair<size_t, size_t> 
   write(const asio::mutable_buffer& buffer, Fields& fields, error_code& ec);

   /*
   Returns whether the complete message has been parsed
   */
   bool complete();
};



/*
Read and parse the HTTP headers
*/
template<class SyncStream, class Parser, class ReadDynamicBufferSequence, class Fields>
void
read_headers(SyncStream& stream, Parser& parser, ContinuousDynamicBuffer& read_buf, Fields& fields, error_code& ec)
{
   for (auto bp = parser.write(read_buf.data(), fields, ec), read_buf.consume(bp.first);
        !parser.complete() && ec == error::need_more();
        bt = parser.write(read_buf.data(), fields, ec), read_buf.consume(bp.first))
   {
      auto const rs = read_size(read_buf);
      ec = {};
      const auto br = stream.read_some(read_buf.prepare(rs), ec);
      read_buf.commit(br);
      if (ec)
      {
         return;
      }
   }
}


/*
Read a full HTTP message filling the body into a stream_buf
TODO: Add support for CompletionCondition
*/
template<class SyncStream, class Parser, class ReadConsecutiveDynamicBuffer, class Fields, class BodyConsecutinveDynamicBuffer>
void
read(SyncStream& stream, Parser& parser, ReadConsecutiveDynamicBuffer& read_buf, BodyConsecutinveDynamicBuffer& body, error_code& ec)
{
   read_headers(stream, parser, read_buf, fields, ec);

   if (ec)
   {
      return;
   }

   if (continue_required(fields))
   {
      write_continue(stream);
   }

   const auto ws = prepare_size(body);

   for (auto bp = parser.write(read_buf.data(), fields, body.prepare(ws) ec), read_buf.consume(bp.first), body.commit(bp.second);
        !parser.complete() && (ec == error::need_more() || ec == error::have_more());
        bt = parser.write(read_buf.data(), fields, ec), readbuf.consume(bp.first), body.commit(bp.second))
   {
      if (ec == error::need_more())
      {
         auto const rs = prepare_size(read_buf);
         ec = {};
         const auto br = stream.read_some(read_buf.prepare(rs), ec);
         read_buf.commit(br);
         if (ec)
         {
            return;
         }
      }
   }
}
