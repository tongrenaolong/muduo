#pragma once

#include "muduo/base/noncopyable.h"
#include "muduo/net/Buffer.h"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <zlib.h>
#include <stdio.h>
#include <iostream>
using namespace std;

namespace muduo
{
namespace net
{

// input is zlib compressed data, output uncompressed data
// FIXME: finish this
class ZlibInputStream : noncopyable
{
 public:
  explicit ZlibInputStream(Buffer* output)
    : output_(output),
      zerror_(Z_OK)
  {
    memZero(&zstream_, sizeof zstream_);
    zerror_ = inflateInit(&zstream_);
  }

  ~ZlibInputStream()
  {
    finish();
  }

  bool write(StringPiece buf);
  bool write(Buffer* input)
  {
    printf("zerror_: %d, Z_OK: %d\n", zerror_,Z_OK );
    if (zerror_ != Z_OK) {
      return false;
    }

    zstream_.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input->peek()));
    zstream_.avail_in = static_cast<int>(input->readableBytes());

    while (zstream_.avail_in > 0 && zerror_ == Z_OK) {
      // 确保输出缓冲区有足够空间
      output_->ensureWritableBytes(1024);
      zstream_.next_out = reinterpret_cast<Bytef*>(output_->beginWrite());
      zstream_.avail_out = static_cast<int>(output_->writableBytes());
      zerror_ = decompress(Z_NO_FLUSH);
      if (zerror_ == Z_OK || zerror_ == Z_STREAM_END) {
        // 更新已写入的数据量
        output_->hasWritten(zstream_.next_out - reinterpret_cast<Bytef*>(output_->beginWrite()));
      }
    }
    std::cout << "Received from client: " << uncompressed.retrieveAllAsString() << std::endl;
    // 将解压后的数据追加到 input 缓冲区
    input->append(output_->peek(), output_->readableBytes());
    output_->retrieveAll();

    input->retrieve(input->readableBytes() - zstream_.avail_in);
    return zerror_ == Z_OK || zerror_ == Z_STREAM_END;
  }
  bool finish()
  {
    if (zerror_ != Z_OK) {
      return false;
    }
    zerror_ = inflateEnd(&zstream_);
    bool ok = zerror_ == Z_OK;
    zerror_ = Z_STREAM_END;
    return ok;
  }

 private:
  int decompress(int flush) {
    output_->ensureWritableBytes(1024);
    zstream_.next_out = reinterpret_cast<Bytef*>(output_->beginWrite());
    zstream_.avail_out = static_cast<int>(output_->writableBytes());
    int result = ::inflate(&zstream_, flush);
    output_->hasWritten(output_->writableBytes() - zstream_.avail_out);
    return result;
  }

  Buffer* output_;
  z_stream zstream_;
  int zerror_;
};

// input is uncompressed data, output zlib compressed data
class ZlibOutputStream : noncopyable
{
 public:
  explicit ZlibOutputStream(Buffer* output)
    : output_(output),
      zerror_(Z_OK),
      bufferSize_(1024)
  {
    memZero(&zstream_, sizeof zstream_);
    zerror_ = deflateInit(&zstream_, Z_DEFAULT_COMPRESSION);
  }

  ~ZlibOutputStream()
  {
    finish();
  }

  // Return last error message or NULL if no error.
  const char* zlibErrorMessage() const { return zstream_.msg; }

  int zlibErrorCode() const { return zerror_; }
  int64_t inputBytes() const { return zstream_.total_in; }
  int64_t outputBytes() const { return zstream_.total_out; }
  int internalOutputBufferSize() const { return bufferSize_; }

  bool write(StringPiece buf)
  {
    if (zerror_ != Z_OK)
      return false;

    assert(zstream_.next_in == NULL && zstream_.avail_in == 0);
    void* in = const_cast<char*>(buf.data());
    zstream_.next_in = static_cast<Bytef*>(in);
    zstream_.avail_in = buf.size();
    while (zstream_.avail_in > 0 && zerror_ == Z_OK)
    {
      zerror_ = compress(Z_NO_FLUSH);
    }
    if (zstream_.avail_in == 0)
    {
      assert(static_cast<const void*>(zstream_.next_in) == buf.end());
      zstream_.next_in = NULL;
    }
    return zerror_ == Z_OK;
  }

  // compress input as much as possible, not guarantee consuming all data.
  bool write(Buffer* input)
  {
    if (zerror_ != Z_OK)
      return false;

    void* in = const_cast<char*>(input->peek());
    zstream_.next_in = static_cast<Bytef*>(in);
    zstream_.avail_in = static_cast<int>(input->readableBytes());
    if (zstream_.avail_in > 0 && zerror_ == Z_OK)
    {
      zerror_ = compress(Z_NO_FLUSH);
    }
    input->retrieve(input->readableBytes() - zstream_.avail_in);
    return zerror_ == Z_OK;
  }

  bool finish()
  {
    if (zerror_ != Z_OK)
      return false;

    while (zerror_ == Z_OK)
    {
      zerror_ = compress(Z_FINISH);
    }
    zerror_ = deflateEnd(&zstream_);
    bool ok = zerror_ == Z_OK;
    zerror_ = Z_STREAM_END;
    return ok;
  }

 private:
  int compress(int flush)
  {
    output_->ensureWritableBytes(bufferSize_);
    zstream_.next_out = reinterpret_cast<Bytef*>(output_->beginWrite());
    zstream_.avail_out = static_cast<int>(output_->writableBytes());
    int error = ::deflate(&zstream_, flush);
    output_->hasWritten(output_->writableBytes() - zstream_.avail_out);
    if (output_->writableBytes() == 0 && bufferSize_ < 65536)
    {
      bufferSize_ *= 2;
    }
    return error;
  }

  Buffer* output_;
  z_stream zstream_;
  int zerror_;
  int bufferSize_;
};

}  // namespace net
}  // namespace muduo
