// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

//this file includes some part of APNG Disassembler 2.9 and APNG Optimizer 1.4 

/* APNG Disassembler 2.9
 *
 * Deconstructs APNG files into individual frames.
 *
 * http://apngdis.sourceforge.net
 *
 * Copyright (c) 2010-2017 Max Stepin
 * maxst at users.sourceforge.net
 *
 * zlib license
 * ------------
*/

/* APNG Optimizer 1.4
 *
 * Makes APNG files smaller.
 *
 * http://sourceforge.net/projects/apng/files
 *
 * Copyright (c) 2011-2015 Max Stepin
 * maxst at users.sourceforge.net
 *
 * zlib license
 * ------------
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "png.h"
#include "zlib.h"

#include "grfmt_png.hpp"

#if defined _MSC_VER && _MSC_VER >= 1200
 // interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning( disable: 4611 )
#pragma warning( disable: 4244 )
#endif

#define notabc(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))

#define id_IHDR 0x52444849
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549

#ifndef PNG_USER_CHUNK_MALLOC_MAX
#define PNG_USER_CHUNK_MALLOC_MAX 8000000
#endif

struct CHUNK { unsigned char* p; unsigned int size; };

const unsigned long cMaxPNGSize = 16384UL;

void info_fn(png_structp png_ptr, png_infop info_ptr);
void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass);
void compose_frame(unsigned char** rows_dst, unsigned char** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
int processing_start(png_structp& png_ptr, png_infop& info_ptr, void* frame_ptr, bool hasInfo, CHUNK& chunkIHDR, std::vector<CHUNK>& chunksInfo);
int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char* p, unsigned int size);
int processing_finish(png_structp png_ptr, png_infop info_ptr);
void deflate_rect_op(unsigned char* pdata, int x, int y, int w, int h, int bpp, int stride, int zbuf_size, int n);

void info_fn(png_structp png_ptr, png_infop info_ptr)
{
  png_set_expand(png_ptr);
  png_set_strip_16(png_ptr);
  png_set_gray_to_rgb(png_ptr);
  png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
  (void)png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
}

void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
    Image* image = (Image*)png_get_progressive_ptr(png_ptr);
    png_progressive_combine_row(png_ptr, image->rows[row_num], new_row);
}

void compose_frame(unsigned char ** rows_dst, unsigned char ** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  unsigned int  i, j;
  int u, v, al;

  for (j=0; j<h; j++)
  {
    unsigned char * sp = rows_src[j];
    unsigned char * dp = rows_dst[j+y] + x*4;

    if (bop == 0)
      memcpy(dp, sp, w*4);
    else
    for (i=0; i<w; i++, sp+=4, dp+=4)
    {
      if (sp[3] == 255)
        memcpy(dp, sp, 4);
      else
      if (sp[3] != 0)
      {
        if (dp[3] != 0)
        {
          u = sp[3]*255;
          v = (255-sp[3])*dp[3];
          al = u + v;
          dp[0] = (sp[0]*u + dp[0]*v)/al;
          dp[1] = (sp[1]*u + dp[1]*v)/al;
          dp[2] = (sp[2]*u + dp[2]*v)/al;
          dp[3] = al/255;
        }
        else
          memcpy(dp, sp, 4);
      }
    }
  }
}

inline unsigned int read_chunk(FILE * f, CHUNK * pChunk)
{
  unsigned char len[4];
  pChunk->size = 0;
  pChunk->p = 0;
  if (fread(&len, 4, 1, f) == 1)
  {
    pChunk->size = png_get_uint_32(len);
    if (pChunk->size > PNG_USER_CHUNK_MALLOC_MAX)
      return 0;
    pChunk->size += 12;
    pChunk->p = new unsigned char[pChunk->size];
    memcpy(pChunk->p, len, 4);
    if (fread(pChunk->p + 4, pChunk->size - 4, 1, f) == 1)
      return *(unsigned int *)(pChunk->p + 4);
  }
  return 0;
}

int processing_start(png_structp & png_ptr, png_infop & info_ptr, void * frame_ptr, bool hasInfo, CHUNK & chunkIHDR, std::vector<CHUNK>& chunksInfo)
{
  unsigned char header[8] = {137, 80, 78, 71, 13, 10, 26, 10};

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
  png_set_progressive_read_fn(png_ptr, frame_ptr, info_fn, row_fn, NULL);

  png_process_data(png_ptr, info_ptr, header, 8);
  png_process_data(png_ptr, info_ptr, chunkIHDR.p, chunkIHDR.size);

  if (hasInfo)
    for (unsigned int i=0; i<chunksInfo.size(); i++)
      png_process_data(png_ptr, info_ptr, chunksInfo[i].p, chunksInfo[i].size);

  return 0;
}

int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char * p, unsigned int size)
{
  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_process_data(png_ptr, info_ptr, p, size);
  return 0;
}

int processing_finish(png_structp png_ptr, png_infop info_ptr)
{
  unsigned char footer[12] = {0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};

  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_process_data(png_ptr, info_ptr, footer, 12);
  png_destroy_read_struct(&png_ptr, &info_ptr, 0);

  return 0;
}

int load_apng(std::string inputFileName, std::vector<Image>& img)
{
  FILE * f;
  unsigned int id, i, j, w, h, w0, h0, x0, y0;
  unsigned int delay_num, delay_den, dop, bop, imagesize;
  unsigned char sig[8];
  png_structp png_ptr;
  png_infop info_ptr;
  CHUNK chunk;
  CHUNK chunkIHDR;
  std::vector<CHUNK> chunksInfo;
  bool isAnimated = false;
  bool skipFirst = false;
  bool hasInfo = false;
  Image frameRaw;
  Image frameCur;
  Image frameNext;
  int res = -1;


  if ((f = fopen(inputFileName.c_str(), "rb")) != 0)
  {
    if (fread(sig, 1, 8, f) == 8 && png_sig_cmp(sig, 0, 8) == 0)
    {
      id = read_chunk(f, &chunkIHDR);
      if (!id)
      {
        fclose(f);
        return res;
      }

      if (id == id_IHDR && chunkIHDR.size == 25)
      {
        w0 = w = png_get_uint_32(chunkIHDR.p + 8);
        h0 = h = png_get_uint_32(chunkIHDR.p + 12);

        if (!w || w > cMaxPNGSize || !h || h > cMaxPNGSize)
        {
          fclose(f);
          return res;
        }

        x0 = 0;
        y0 = 0;
        delay_num = 1;
        delay_den = 10;
        dop = 0;
        bop = 0;
        imagesize = w * h * 4;

        frameRaw.init(w, h, 4);

        if (!processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
        {
          frameCur.init(w, h, 4);

          while ( !feof(f) )
          {
            id = read_chunk(f, &chunk);
            if (!id)
              break;

            if (id == id_acTL && !hasInfo && !isAnimated)
            {
              isAnimated = true;
              skipFirst = true;
            }
            else
            if (id == id_fcTL && (!hasInfo || isAnimated))
            {
              if (hasInfo)
              {
                if (!processing_finish(png_ptr, info_ptr))
                {
                  frameNext.init(w, h, 4);

                  if (dop == 2)
                    memcpy(frameNext.p, frameCur.p, imagesize);

                  compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                  frameCur.delay_num = delay_num;
                  frameCur.delay_den = delay_den;
                  img.push_back(frameCur);

                  if (dop != 2)
                  {
                    memcpy(frameNext.p, frameCur.p, imagesize);
                    if (dop == 1)
                      for (j=0; j<h0; j++)
                        memset(frameNext.rows[y0 + j] + x0*4, 0, w0*4);
                  }
                  frameCur.p = frameNext.p;
                  frameCur.rows = frameNext.rows;
                }
                else
                {
                  frameCur.free();
                  delete[] chunk.p;
                  break;
                }
              }

              // At this point the old frame is done. Let's start a new one.
              w0 = png_get_uint_32(chunk.p + 12);
              h0 = png_get_uint_32(chunk.p + 16);
              x0 = png_get_uint_32(chunk.p + 20);
              y0 = png_get_uint_32(chunk.p + 24);
              delay_num = png_get_uint_16(chunk.p + 28);
              delay_den = png_get_uint_16(chunk.p + 30);
              dop = chunk.p[32];
              bop = chunk.p[33];

              if (!w0 || w0 > cMaxPNGSize || !h0 || h0 > cMaxPNGSize
                  || x0 + w0 > w || y0 + h0 > h || dop > 2 || bop > 1)
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }

              if (hasInfo)
              {
                memcpy(chunkIHDR.p + 8, chunk.p + 12, 8);
                if (processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
                {
                  frameCur.free();
                  delete[] chunk.p;
                  break;
                }
              }
              else
                skipFirst = false;

              if (img.size() == (skipFirst ? 1 : 0))
              {
                bop = 0;
                if (dop == 2)
                  dop = 1;
              }
            }
            else
            if (id == id_IDAT)
            {
              hasInfo = true;
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_fdAT && isAnimated)
            {
              png_save_uint_32(chunk.p + 4, chunk.size - 16);
              memcpy(chunk.p + 8, "IDAT", 4);
              if (processing_data(png_ptr, info_ptr, chunk.p + 4, chunk.size - 4))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_IEND)
            {
              if (hasInfo && !processing_finish(png_ptr, info_ptr))
              {
                compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                frameCur.delay_num = delay_num;
                frameCur.delay_den = delay_den;
                img.push_back(frameCur);
              }
              else
                frameCur.free();

              delete[] chunk.p;
              break;
            }
            else
            if (notabc(chunk.p[4]) || notabc(chunk.p[5]) || notabc(chunk.p[6]) || notabc(chunk.p[7]))
            {
              delete[] chunk.p;
              break;
            }
            else
            if (!hasInfo)
            {
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
              chunksInfo.push_back(chunk);
              continue;
            }
            delete[] chunk.p;
          }
        }
        frameRaw.free();

        if (!img.empty())
          res = (skipFirst) ? 0 : 1;
      }

      for (i=0; i<chunksInfo.size(); i++)
        delete[] chunksInfo[i].p;

      chunksInfo.clear();
      delete[] chunkIHDR.p;
    }
    fclose(f);
  }

  return res;
}

struct OP { unsigned char* p; unsigned int size; int x, y, w, h, valid, filters; };
struct rgb { unsigned char r, g, b; };

unsigned char* op_zbuf1;
unsigned char* op_zbuf2;
z_stream        op_zstream1;
z_stream        op_zstream2;
unsigned char* row_buf;
unsigned char* sub_row;
unsigned char* up_row;
unsigned char* avg_row;
unsigned char* paeth_row;
OP              op[6];
rgb             palette[256];
unsigned char   trns[256];
unsigned int    palsize, trnssize;
unsigned int    next_seq_num;

void write_chunk(FILE* f, const char* name, unsigned char* data, unsigned int length)
{
    unsigned char buf[4];
    unsigned int crc = crc32(0, Z_NULL, 0);

    png_save_uint_32(buf, length);
    fwrite(buf, 1, 4, f);
    fwrite(name, 1, 4, f);
    crc = crc32(crc, (const Bytef*)name, 4);

    if (memcmp(name, "fdAT", 4) == 0)
    {
        png_save_uint_32(buf, next_seq_num++);
        fwrite(buf, 1, 4, f);
        crc = crc32(crc, buf, 4);
        length -= 4;
    }

    if (data != NULL && length > 0)
    {
        fwrite(data, 1, length, f);
        crc = crc32(crc, data, length);
    }

    png_save_uint_32(buf, crc);
    fwrite(buf, 1, 4, f);
}

void write_IDATs(FILE* f, int frame, unsigned char* data, unsigned int length, unsigned int idat_size)
{
    unsigned int z_cmf = data[0];
    if ((z_cmf & 0x0f) == 8 && (z_cmf & 0xf0) <= 0x70)
    {
        if (length >= 2)
        {
            unsigned int z_cinfo = z_cmf >> 4;
            unsigned int half_z_window_size = 1 << (z_cinfo + 7);
            while (idat_size <= half_z_window_size && half_z_window_size >= 256)
            {
                z_cinfo--;
                half_z_window_size >>= 1;
            }
            z_cmf = (z_cmf & 0x0f) | (z_cinfo << 4);
            if (data[0] != (unsigned char)z_cmf)
            {
                data[0] = (unsigned char)z_cmf;
                data[1] &= 0xe0;
                data[1] += (unsigned char)(0x1f - ((z_cmf << 8) + data[1]) % 0x1f);
            }
        }
    }

    while (length > 0)
    {
        unsigned int ds = length;
        if (ds > 32768)
            ds = 32768;

        if (frame == 0)
            write_chunk(f, "IDAT", data, ds);
        else
            write_chunk(f, "fdAT", data, ds + 4);

        data += ds;
        length -= ds;
    }
}

void process_rect(unsigned char* row, int rowbytes, int bpp, int stride, int h, unsigned char* rows)
{
    int i, j, v;
    int a, b, c, pa, pb, pc, p;
    unsigned char* prev = NULL;
    unsigned char* dp = rows;
    unsigned char* out;

    for (j = 0; j < h; j++)
    {
        unsigned int    sum = 0;
        unsigned char* best_row = row_buf;
        unsigned int    mins = ((unsigned int)(-1)) >> 1;

        out = row_buf + 1;
        for (i = 0; i < rowbytes; i++)
        {
            v = out[i] = row[i];
            sum += (v < 128) ? v : 256 - v;
        }
        mins = sum;

        sum = 0;
        out = sub_row + 1;
        for (i = 0; i < bpp; i++)
        {
            v = out[i] = row[i];
            sum += (v < 128) ? v : 256 - v;
        }
        for (i = bpp; i < rowbytes; i++)
        {
            v = out[i] = row[i] - row[i - bpp];
            sum += (v < 128) ? v : 256 - v;
            if (sum > mins) break;
        }
        if (sum < mins)
        {
            mins = sum;
            best_row = sub_row;
        }

        if (prev)
        {
            sum = 0;
            out = up_row + 1;
            for (i = 0; i < rowbytes; i++)
            {
                v = out[i] = row[i] - prev[i];
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins) break;
            }
            if (sum < mins)
            {
                mins = sum;
                best_row = up_row;
            }

            sum = 0;
            out = avg_row + 1;
            for (i = 0; i < bpp; i++)
            {
                v = out[i] = row[i] - prev[i] / 2;
                sum += (v < 128) ? v : 256 - v;
            }
            for (i = bpp; i < rowbytes; i++)
            {
                v = out[i] = row[i] - (prev[i] + row[i - bpp]) / 2;
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins) break;
            }
            if (sum < mins)
            {
                mins = sum;
                best_row = avg_row;
            }

            sum = 0;
            out = paeth_row + 1;
            for (i = 0; i < bpp; i++)
            {
                v = out[i] = row[i] - prev[i];
                sum += (v < 128) ? v : 256 - v;
            }
            for (i = bpp; i < rowbytes; i++)
            {
                a = row[i - bpp];
                b = prev[i];
                c = prev[i - bpp];
                p = b - c;
                pc = a - c;
                pa = abs(p);
                pb = abs(pc);
                pc = abs(p + pc);
                p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
                v = out[i] = row[i] - p;
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins) break;
            }
            if (sum < mins)
            {
                best_row = paeth_row;
            }
        }

        if (rows == NULL)
        {
            // deflate_rect_op()
            op_zstream1.next_in = row_buf;
            op_zstream1.avail_in = rowbytes + 1;
            deflate(&op_zstream1, Z_NO_FLUSH);

            op_zstream2.next_in = best_row;
            op_zstream2.avail_in = rowbytes + 1;
            deflate(&op_zstream2, Z_NO_FLUSH);
        }
        else
        {
            // deflate_rect_fin()
            memcpy(dp, best_row, rowbytes + 1);
            dp += rowbytes + 1;
        }

        prev = row;
        row += stride;
    }
}

void deflate_rect_op(unsigned char* pdata, int x, int y, int w, int h, int bpp, int stride, int zbuf_size, int n)
{
    unsigned char* row = pdata + y * stride + x * bpp;
    int rowbytes = w * bpp;

    op_zstream1.data_type = Z_BINARY;
    op_zstream1.next_out = op_zbuf1;
    op_zstream1.avail_out = zbuf_size;

    op_zstream2.data_type = Z_BINARY;
    op_zstream2.next_out = op_zbuf2;
    op_zstream2.avail_out = zbuf_size;

    process_rect(row, rowbytes, bpp, stride, h, NULL);

    deflate(&op_zstream1, Z_FINISH);
    deflate(&op_zstream2, Z_FINISH);
    op[n].p = pdata;

    if (op_zstream1.total_out < op_zstream2.total_out)
    {
        op[n].size = op_zstream1.total_out;
        op[n].filters = 0;
    }
    else
    {
        op[n].size = op_zstream2.total_out;
        op[n].filters = 1;
    }
    op[n].x = x;
    op[n].y = y;
    op[n].w = w;
    op[n].h = h;
    op[n].valid = 1;
    deflateReset(&op_zstream1);
    deflateReset(&op_zstream2);
}

void get_rect(unsigned int w, unsigned int h, unsigned char* pimage1, unsigned char* pimage2, unsigned char* ptemp, unsigned int bpp, unsigned int stride, int zbuf_size, unsigned int has_tcolor, unsigned int tcolor, int n)
{
    unsigned int   i, j, x0, y0, w0, h0;
    unsigned int   x_min = w - 1;
    unsigned int   y_min = h - 1;
    unsigned int   x_max = 0;
    unsigned int   y_max = 0;
    unsigned int   diffnum = 0;
    unsigned int   over_is_possible = 1;

    if (!has_tcolor)
        over_is_possible = 0;

    if (bpp == 1)
    {
        unsigned char* pa = pimage1;
        unsigned char* pb = pimage2;
        unsigned char* pc = ptemp;

        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++)
            {
                unsigned char c = *pb++;
                if (*pa++ != c)
                {
                    diffnum++;
                    if (has_tcolor && c == tcolor) over_is_possible = 0;
                    if (i < x_min) x_min = i;
                    if (i > x_max) x_max = i;
                    if (j < y_min) y_min = j;
                    if (j > y_max) y_max = j;
                }
                else
                    c = tcolor;

                *pc++ = c;
            }
    }
    else
        if (bpp == 2)
        {
            unsigned short* pa = (unsigned short*)pimage1;
            unsigned short* pb = (unsigned short*)pimage2;
            unsigned short* pc = (unsigned short*)ptemp;

            for (j = 0; j < h; j++)
                for (i = 0; i < w; i++)
                {
                    unsigned int c1 = *pa++;
                    unsigned int c2 = *pb++;
                    if ((c1 != c2) && ((c1 >> 8) || (c2 >> 8)))
                    {
                        diffnum++;
                        if ((c2 >> 8) != 0xFF) over_is_possible = 0;
                        if (i < x_min) x_min = i;
                        if (i > x_max) x_max = i;
                        if (j < y_min) y_min = j;
                        if (j > y_max) y_max = j;
                    }
                    else
                        c2 = 0;

                    *pc++ = c2;
                }
        }
        else
            if (bpp == 3)
            {
                unsigned char* pa = pimage1;
                unsigned char* pb = pimage2;
                unsigned char* pc = ptemp;

                for (j = 0; j < h; j++)
                    for (i = 0; i < w; i++)
                    {
                        unsigned int c1 = (pa[2] << 16) + (pa[1] << 8) + pa[0];
                        unsigned int c2 = (pb[2] << 16) + (pb[1] << 8) + pb[0];
                        if (c1 != c2)
                        {
                            diffnum++;
                            if (has_tcolor && c2 == tcolor) over_is_possible = 0;
                            if (i < x_min) x_min = i;
                            if (i > x_max) x_max = i;
                            if (j < y_min) y_min = j;
                            if (j > y_max) y_max = j;
                        }
                        else
                            c2 = tcolor;

                        memcpy(pc, &c2, 3);
                        pa += 3;
                        pb += 3;
                        pc += 3;
                    }
            }
            else
                if (bpp == 4)
                {
                    unsigned int* pa = (unsigned int*)pimage1;
                    unsigned int* pb = (unsigned int*)pimage2;
                    unsigned int* pc = (unsigned int*)ptemp;

                    for (j = 0; j < h; j++)
                        for (i = 0; i < w; i++)
                        {
                            unsigned int c1 = *pa++;
                            unsigned int c2 = *pb++;
                            if ((c1 != c2) && ((c1 >> 24) || (c2 >> 24)))
                            {
                                diffnum++;
                                if ((c2 >> 24) != 0xFF) over_is_possible = 0;
                                if (i < x_min) x_min = i;
                                if (i > x_max) x_max = i;
                                if (j < y_min) y_min = j;
                                if (j > y_max) y_max = j;
                            }
                            else
                                c2 = 0;

                            *pc++ = c2;
                        }
                }

    if (diffnum == 0)
    {
        x0 = y0 = 0;
        w0 = h0 = 1;
    }
    else
    {
        x0 = x_min;
        y0 = y_min;
        w0 = x_max - x_min + 1;
        h0 = y_max - y_min + 1;
    }

    deflate_rect_op(pimage2, x0, y0, w0, h0, bpp, stride, zbuf_size, n * 2);

    if (over_is_possible)
        deflate_rect_op(ptemp, x0, y0, w0, h0, bpp, stride, zbuf_size, n * 2 + 1);
}

void deflate_rect_fin(int deflate_method, int iter, unsigned char* zbuf, unsigned int* zsize, int bpp, int stride, unsigned char* rows, int zbuf_size, int n)
{
    unsigned char* row = op[n].p + op[n].y * stride + op[n].x * bpp;
    int rowbytes = op[n].w * bpp;

    if (op[n].filters == 0)
    {
        unsigned char* dp = rows;
        for (int j = 0; j < op[n].h; j++)
        {
            *dp++ = 0;
            memcpy(dp, row, rowbytes);
            dp += rowbytes;
            row += stride;
        }
    }
    else
        process_rect(row, rowbytes, bpp, stride, op[n].h, rows);

    if (deflate_method == 2)
    {

    }
    else
        if (deflate_method == 1)
        {
        }
        else
        {
            z_stream fin_zstream;

            fin_zstream.data_type = Z_BINARY;
            fin_zstream.zalloc = Z_NULL;
            fin_zstream.zfree = Z_NULL;
            fin_zstream.opaque = Z_NULL;
            deflateInit2(&fin_zstream, Z_BEST_COMPRESSION, 8, 15, 8, op[n].filters ? Z_FILTERED : Z_DEFAULT_STRATEGY);

            fin_zstream.next_out = zbuf;
            fin_zstream.avail_out = zbuf_size;
            fin_zstream.next_in = rows;
            fin_zstream.avail_in = op[n].h * (rowbytes + 1);
            deflate(&fin_zstream, Z_FINISH);
            *zsize = fin_zstream.total_out;
            deflateEnd(&fin_zstream);
        }
}

int save_apng(std::string outputFileName, std::vector<Image>& frames, unsigned int first, unsigned int loops, unsigned int coltype, int deflate_method, int iter)
{
    FILE* f;
    unsigned int i, j, k;
    unsigned int x0, y0, w0, h0, dop, bop;
    unsigned int idat_size, zbuf_size, zsize;
    unsigned char* zbuf;
    unsigned char header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    unsigned int num_frames = frames.size();
    unsigned int width = frames[0].w;
    unsigned int height = frames[0].h;
    unsigned int bpp = (coltype == 6) ? 4 : (coltype == 2) ? 3 : (coltype == 4) ? 2 : 1;
    unsigned int has_tcolor = (coltype >= 4 || (coltype <= 2 && trnssize)) ? 1 : 0;
    unsigned int tcolor = 0;
    unsigned int rowbytes = width * bpp;
    unsigned int imagesize = rowbytes * height;

    unsigned char* temp = new unsigned char[imagesize];
    unsigned char* over1 = new unsigned char[imagesize];
    unsigned char* over2 = new unsigned char[imagesize];
    unsigned char* over3 = new unsigned char[imagesize];
    unsigned char* rest = new unsigned char[imagesize];
    unsigned char* rows = new unsigned char[(rowbytes + 1) * height];

    if (trnssize)
    {
        if (coltype == 0)
            tcolor = trns[1];
        else
            if (coltype == 2)
                tcolor = (((trns[5] << 8) + trns[3]) << 8) + trns[1];
            else
                if (coltype == 3)
                {
                    for (i = 0; i < trnssize; i++)
                        if (trns[i] == 0)
                        {
                            has_tcolor = 1;
                            tcolor = i;
                            break;
                        }
                }
    }

    if ((f = fopen(outputFileName.c_str(), "wb")) != 0)
    {
        unsigned char buf_IHDR[13];
        unsigned char buf_acTL[8];
        unsigned char buf_fcTL[26];

        png_save_uint_32(buf_IHDR, width);
        png_save_uint_32(buf_IHDR + 4, height);
        buf_IHDR[8] = 8;
        buf_IHDR[9] = coltype;
        buf_IHDR[10] = 0;
        buf_IHDR[11] = 0;
        buf_IHDR[12] = 0;

        png_save_uint_32(buf_acTL, num_frames - first);
        png_save_uint_32(buf_acTL + 4, loops);

        fwrite(header, 1, 8, f);

        write_chunk(f, "IHDR", buf_IHDR, 13);

        if (num_frames > 1)
            write_chunk(f, "acTL", buf_acTL, 8);
        else
            first = 0;

        if (palsize > 0)
            write_chunk(f, "PLTE", (unsigned char*)(&palette), palsize * 3);

        if (trnssize > 0)
            write_chunk(f, "tRNS", trns, trnssize);

        op_zstream1.data_type = Z_BINARY;
        op_zstream1.zalloc = Z_NULL;
        op_zstream1.zfree = Z_NULL;
        op_zstream1.opaque = Z_NULL;
        deflateInit2(&op_zstream1, Z_BEST_SPEED + 1, 8, 15, 8, Z_DEFAULT_STRATEGY);

        op_zstream2.data_type = Z_BINARY;
        op_zstream2.zalloc = Z_NULL;
        op_zstream2.zfree = Z_NULL;
        op_zstream2.opaque = Z_NULL;
        deflateInit2(&op_zstream2, Z_BEST_SPEED + 1, 8, 15, 8, Z_FILTERED);

        idat_size = (rowbytes + 1) * height;
        zbuf_size = idat_size + ((idat_size + 7) >> 3) + ((idat_size + 63) >> 6) + 11;

        zbuf = new unsigned char[zbuf_size];
        op_zbuf1 = new unsigned char[zbuf_size];
        op_zbuf2 = new unsigned char[zbuf_size];
        row_buf = new unsigned char[rowbytes + 1];
        sub_row = new unsigned char[rowbytes + 1];
        up_row = new unsigned char[rowbytes + 1];
        avg_row = new unsigned char[rowbytes + 1];
        paeth_row = new unsigned char[rowbytes + 1];

        row_buf[0] = 0;
        sub_row[0] = 1;
        up_row[0] = 2;
        avg_row[0] = 3;
        paeth_row[0] = 4;

        x0 = 0;
        y0 = 0;
        w0 = width;
        h0 = height;
        bop = 0;
        next_seq_num = 0;

        //printf("saving %s (frame %d of %d)\n", outputFileName.c_str(), 1 - first, num_frames - first);
        for (j = 0; j < 6; j++)
            op[j].valid = 0;
        deflate_rect_op(frames[0].p, x0, y0, w0, h0, bpp, rowbytes, zbuf_size, 0);
        deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, 0);

        if (first)
        {
            write_IDATs(f, 0, zbuf, zsize, idat_size);

            //printf("saving %s (frame %d of %d)\n", outputFileName.c_str(), 1, num_frames - first);
            for (j = 0; j < 6; j++)
                op[j].valid = 0;
            deflate_rect_op(frames[1].p, x0, y0, w0, h0, bpp, rowbytes, zbuf_size, 0);
            deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, 0);
        }

        for (i = first; i < num_frames - 1; i++)
        {
            unsigned int op_min;
            int          op_best;

            //printf("saving %s (frame %d of %d)\n", outputFileName.c_str(), i - first + 2, num_frames - first);
            for (j = 0; j < 6; j++)
                op[j].valid = 0;

            /* dispose = none */
            get_rect(width, height, frames[i].p, frames[i + 1].p, over1, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 0);

            /* dispose = background */
            if (has_tcolor)
            {
                memcpy(temp, frames[i].p, imagesize);
                if (coltype == 2)
                    for (j = 0; j < h0; j++)
                        for (k = 0; k < w0; k++)
                            memcpy(temp + ((j + y0) * width + (k + x0)) * 3, &tcolor, 3);
                else
                    for (j = 0; j < h0; j++)
                        memset(temp + ((j + y0) * width + x0) * bpp, tcolor, w0 * bpp);

                get_rect(width, height, temp, frames[i + 1].p, over2, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 1);
            }

            /* dispose = previous */
            if (i > first)
                get_rect(width, height, rest, frames[i + 1].p, over3, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 2);

            op_min = op[0].size;
            op_best = 0;
            for (j = 1; j < 6; j++)
                if (op[j].valid)
                {
                    if (op[j].size < op_min)
                    {
                        op_min = op[j].size;
                        op_best = j;
                    }
                }

            dop = op_best >> 1;

            png_save_uint_32(buf_fcTL, next_seq_num++);
            png_save_uint_32(buf_fcTL + 4, w0);
            png_save_uint_32(buf_fcTL + 8, h0);
            png_save_uint_32(buf_fcTL + 12, x0);
            png_save_uint_32(buf_fcTL + 16, y0);
            png_save_uint_16(buf_fcTL + 20, frames[i].delay_num);
            png_save_uint_16(buf_fcTL + 22, frames[i].delay_den);
            buf_fcTL[24] = dop;
            buf_fcTL[25] = bop;
            write_chunk(f, "fcTL", buf_fcTL, 26);

            write_IDATs(f, i, zbuf, zsize, idat_size);

            /* process apng dispose - begin */
            if (dop != 2)
                memcpy(rest, frames[i].p, imagesize);

            if (dop == 1)
            {
                if (coltype == 2)
                    for (j = 0; j < h0; j++)
                        for (k = 0; k < w0; k++)
                            memcpy(rest + ((j + y0) * width + (k + x0)) * 3, &tcolor, 3);
                else
                    for (j = 0; j < h0; j++)
                        memset(rest + ((j + y0) * width + x0) * bpp, tcolor, w0 * bpp);
            }
            /* process apng dispose - end */

            x0 = op[op_best].x;
            y0 = op[op_best].y;
            w0 = op[op_best].w;
            h0 = op[op_best].h;
            bop = op_best & 1;

            deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, op_best);
        }

        if (num_frames > 1)
        {
            png_save_uint_32(buf_fcTL, next_seq_num++);
            png_save_uint_32(buf_fcTL + 4, w0);
            png_save_uint_32(buf_fcTL + 8, h0);
            png_save_uint_32(buf_fcTL + 12, x0);
            png_save_uint_32(buf_fcTL + 16, y0);
            png_save_uint_16(buf_fcTL + 20, frames[num_frames - 1].delay_num);
            png_save_uint_16(buf_fcTL + 22, frames[num_frames - 1].delay_den);
            buf_fcTL[24] = 0;
            buf_fcTL[25] = bop;
            write_chunk(f, "fcTL", buf_fcTL, 26);
        }

        write_IDATs(f, num_frames - 1, zbuf, zsize, idat_size);

        write_chunk(f, "IEND", 0, 0);
        fclose(f);

        delete[] zbuf;
        delete[] op_zbuf1;
        delete[] op_zbuf2;
        delete[] row_buf;
        delete[] sub_row;
        delete[] up_row;
        delete[] avg_row;
        delete[] paeth_row;

        deflateEnd(&op_zstream1);
        deflateEnd(&op_zstream2);
    }
    else
    {
        printf("Error: couldn't open file for writing\n");
        return 1;
    }

    delete[] temp;
    delete[] over1;
    delete[] over2;
    delete[] over3;
    delete[] rest;
    delete[] rows;

    return 0;
}
/* APNG encoder - end */
