// Copyright (C) 2006  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_IMAGE_LOADEr_
#define DLIB_IMAGE_LOADEr_

#include "image_loader_abstract.h"
#include <iostream>
#include <sstream>
#include "../algs.h"
#include "../pixel.h"
#include "../image_saver/dng_shared.h"
#include "../entropy_decoder_model.h"
#include "../entropy_decoder.h"
#include "../uintn.h"
#include "../image_transforms/assign_image.h"
#include <algorithm>
#include "../vectorstream.h"

namespace dlib
{

// ----------------------------------------------------------------------------------------

    class image_load_error : public dlib::error { 
    public: image_load_error(const std::string& str) : error(EIMAGE_LOAD,str){}
    };

// ----------------------------------------------------------------------------------------

    template <
        typename image_type 
        >
    void load_bmp (
        image_type& image_,
        std::istream& in_
    )
    {
        image_view<image_type> image(image_);
        try
        {
            unsigned long bytes_read_so_far = 0;
            unsigned long bfSize;
            unsigned long bfOffBits;
            unsigned long bfReserved;
            unsigned long biSize;
            unsigned long biWidth;
            unsigned long biHeight;
            unsigned short biBitCount;
            unsigned long biCompression;
            /*
            unsigned long biSizeImage;
            unsigned long biClrUsed;
            unsigned long biClrImportant;
            */
            unsigned long a, b, c, d, i;

            using namespace std;

            streambuf& in = *in_.rdbuf();
    //        streamsize num;
            unsigned char buf[100];


            // first make sure the BMP starts with BM
            if (in.sgetn(reinterpret_cast<char*>(buf),2) != 2)
                abort();
            bytes_read_so_far += 2;

            if (buf[0] != 'B' || buf[1] != 'M')
                abort();

            // now read the BITMAPFILEHEADER
            if (in.sgetn(reinterpret_cast<char*>(buf),12) != 12)
                abort();

            bytes_read_so_far += 12;

            i = 0;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            bfSize = a | (b<<8) | (c<<16) | (d<<24);

            i = 4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            bfReserved = a | (b<<8) | (c<<16) | (d<<24);

            i = 8;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            bfOffBits = a | (b<<8) | (c<<16) | (d<<24);

            // if this value isn't zero then there is something wrong
            // with this bitmap.
            if (bfReserved != 0)
                abort();


            // load the BITMAPINFOHEADER
            if (in.sgetn(reinterpret_cast<char*>(buf),40) != 40)
                abort();
            bytes_read_so_far += 40;


            i = 0;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biSize = a | (b<<8) | (c<<16) | (d<<24);

            i += 4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biWidth = a | (b<<8) | (c<<16) | (d<<24);

            i += 4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biHeight = a | (b<<8) | (c<<16) | (d<<24);

            i += 4+2;
            a = buf[i]; b = buf[i+1];
            biBitCount = static_cast<unsigned short>(a | (b<<8));

            i += 2;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biCompression = a | (b<<8) | (c<<16) | (d<<24);

            /*
            i += 4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biSizeImage = a | (b<<8) | (c<<16) | (d<<24);

            i += 4+4+4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biClrUsed = a | (b<<8) | (c<<16) | (d<<24);

            i += 4;
            a = buf[i]; b = buf[i+1]; c = buf[i+2]; d = buf[i+3];
            biClrImportant = a | (b<<8) | (c<<16) | (d<<24);
            */


            if (biSize != 40)
                abort();

            // read and discard any extra bytes that are part of the header
            if (biSize > 40)
            {
                if (in.sgetn(reinterpret_cast<char*>(buf),biSize-40) != static_cast<long>(biSize - 40))
                {
                    abort();
                }
                bytes_read_so_far += biSize-40;
            }

            image.set_size(biHeight, biWidth);

            switch (biBitCount)
            {
                case 1:
                    {
                        // figure out how the pixels are packed
                        long padding;
                        if (bfSize - bfOffBits == biWidth*biHeight/8)
                            padding = 0;
                        else
                            padding = 4 - ((biWidth+7)/8)%4;

                        const unsigned int palette_size = 2;
                        unsigned char red[palette_size];
                        unsigned char green[palette_size];
                        unsigned char blue[palette_size];

                        for (unsigned int i = 0; i < palette_size; ++i)
                        {
                            if (in.sgetn(reinterpret_cast<char*>(buf),4) != 4)
                            {
                                abort();
                            }
                            bytes_read_so_far += 4;
                            blue[i] = buf[0];
                            green[i] = buf[1];
                            red[i] = buf[2];
                        }


                        // seek to the start of the pixel data
                        while (bytes_read_so_far != bfOffBits)
                        {
                            const long to_read = (long)std::min(bfOffBits - bytes_read_so_far, (unsigned long)sizeof(buf));
                            if (in.sgetn(reinterpret_cast<char*>(buf), to_read) != to_read)
                            {
                                abort();
                            }
                            bytes_read_so_far += to_read;
                        }

                        // load the image data
                        for (long row = biHeight-1; row >= 0; --row)
                        {
                            for (unsigned long col = 0; col < biWidth; col+=8)
                            {
                                if (in.sgetn(reinterpret_cast<char*>(buf),1) != 1)
                                {
                                    abort();
                                }

                                unsigned char pixels[8];

                                pixels[0] = (buf[0]>>7);
                                pixels[1] = ((buf[0]>>6)&0x01);
                                pixels[2] = ((buf[0]>>5)&0x01);
                                pixels[3] = ((buf[0]>>4)&0x01);
                                pixels[4] = ((buf[0]>>3)&0x01);
                                pixels[5] = ((buf[0]>>2)&0x01);
                                pixels[6] = ((buf[0]>>1)&0x01);
                                pixels[7] = ((buf[0])&0x01);

                                for (int i = 0; i < 8 && col+i < biWidth; ++i)
                                {
                                    rgb_pixel p;
                                    p.red   = red[pixels[i]];
                                    p.green = green[pixels[i]];
                                    p.blue  = blue[pixels[i]];
                                    assign_pixel(image[row][col+i],p);
                                }
                            }
                            if (in.sgetn(reinterpret_cast<char*>(buf),padding) != padding)
                                abort();
                        }



                    } break;
                case 4:
                    {
                        // figure out how the pixels are packed
                        long padding;
                        if (bfSize - bfOffBits == biWidth*biHeight/2)
                            padding = 0;
                        else
                            padding = 4 - ((biWidth+1)/2)%4;

                        const unsigned int palette_size = 16;
                        unsigned char red[palette_size];
                        unsigned char green[palette_size];
                        unsigned char blue[palette_size];

                        for (unsigned int i = 0; i < palette_size; ++i)
                        {
                            if (in.sgetn(reinterpret_cast<char*>(buf),4) != 4)
                            {
                                abort();
                            }
                            bytes_read_so_far += 4;
                            blue[i] = buf[0];
                            green[i] = buf[1];
                            red[i] = buf[2];
                        }


                        // seek to the start of the pixel data
                        while (bytes_read_so_far != bfOffBits)
                        {
                            const long to_read = (long)std::min(bfOffBits - bytes_read_so_far, (unsigned long)sizeof(buf));
                            if (in.sgetn(reinterpret_cast<char*>(buf), to_read) != to_read)
                            {
                                abort();
                            }
                            bytes_read_so_far += to_read;
                        }

                        // load the image data
                        for (long row = biHeight-1; row >= 0; --row)
                        {
                            for (unsigned long col = 0; col < biWidth; col+=2)
                            {
                                if (in.sgetn(reinterpret_cast<char*>(buf),1) != 1)
                                {
                                    abort();
                                }

                                const unsigned char pixel1 = (buf[0]>>4);
                                const unsigned char pixel2 = (buf[0]&0x0F);

                                rgb_pixel p;
                                p.red = red[pixel1];
                                p.green = green[pixel1];
                                p.blue = blue[pixel1];
                                assign_pixel(image[row][col], p);

                                if (col+1 < biWidth)
                                {
                                    p.red   = red[pixel2];
                                    p.green = green[pixel2];
                                    p.blue  = blue[pixel2];
                                    assign_pixel(image[row][col+1], p);
                                }
                            }
                            if (in.sgetn(reinterpret_cast<char*>(buf),padding) != padding)
                                abort();
                        }



                    } break;
                case 8:
                    {
                        // figure out how the pixels are packed
                        long padding;
                        if (bfSize - bfOffBits == biWidth*biHeight)
                            padding = 0;
                        else
                            padding = 4 - biWidth%4;

                        // check for this case.  It shouldn't happen but some BMP writers screw up the files
                        // so we have to do this.
                        if (biHeight*(biWidth+padding) > bfSize - bfOffBits)
                            padding = 0;

                        const unsigned int palette_size = 256;
                        unsigned char red[palette_size];
                        unsigned char green[palette_size];
                        unsigned char blue[palette_size];

                        for (unsigned int i = 0; i < palette_size; ++i)
                        {
                            if (in.sgetn(reinterpret_cast<char*>(buf),4) != 4)
                            {
                                abort();
                            }
                            bytes_read_so_far += 4;
                            blue[i] = buf[0];
                            green[i] = buf[1];
                            red[i] = buf[2];
                        }


                        // seek to the start of the pixel data
                        while (bytes_read_so_far != bfOffBits)
                        {
                            const long to_read = (long)std::min(bfOffBits - bytes_read_so_far, (unsigned long)sizeof(buf));
                            if (in.sgetn(reinterpret_cast<char*>(buf), to_read) != to_read)
                            {
                                abort();
                            }
                            bytes_read_so_far += to_read;
                        }

                        // Next we load the image data.

                        // if there is no RLE compression
                        if (biCompression == 0)
                        {
                            for (long row = biHeight-1; row >= 0; --row)
                            {
                                for (unsigned long col = 0; col < biWidth; ++col)
                                {
                                    if (in.sgetn(reinterpret_cast<char*>(buf),1) != 1)
                                    {
                                        abort();
                                    }

                                    rgb_pixel p;
                                    p.red   = red[buf[0]];
                                    p.green = green[buf[0]];
                                    p.blue  = blue[buf[0]];
                                    assign_pixel(image[row][col],p);
                                }
                                if (in.sgetn(reinterpret_cast<char*>(buf),padding) != padding)
                                    abort();
                            }
                        }
                        else
                        {
                            // Here we deal with the psychotic RLE used by BMP files.

                            // First zero the image since the RLE sometimes jumps over
                            // pixels and assumes the image has been zero initialized.
                            assign_all_pixels(image, 0);

                            long row = biHeight-1;
                            long col = 0;
                            while (true)
                            {
                                if (in.sgetn(reinterpret_cast<char*>(buf),2) != 2)
                                {
                                    abort();
                                }

                                const unsigned char count = buf[0];
                                const unsigned char command = buf[1];

                                if (count == 0 && command == 0)
                                {
                                    // This is an escape code that means go to the next row
                                    // of the image
                                    --row;
                                    col = 0;
                                    continue;
                                }
                                else if (count == 0 && command == 1)
                                {
                                    // This is the end of the image.  So quit this loop.
                                    break;
                                }
                                else if (count == 0 && command == 2)
                                {
                                    // This is the escape code for the command to jump to
                                    // a new part of the image relative to where we are now.
                                    if (in.sgetn(reinterpret_cast<char*>(buf),2) != 2)
                                    {
                                        abort();
                                    }
                                    col += buf[0];
                                    row -= buf[1];
                                    continue;
                                }
                                else if (count == 0)
                                {
                                    // This is the escape code for a run of uncompressed bytes

                                    if (row < 0 || col + command > image.nc())
                                    {
                                        // If this is just some padding bytes at the end then ignore them
                                        if (row >= 0 && col + count <= image.nc() + padding)
                                            continue;

                                        abort();
                                    }

                                    // put the bytes into the image
                                    for (unsigned int i = 0; i < command; ++i)
                                    {
                                        if (in.sgetn(reinterpret_cast<char*>(buf),1) != 1)
                                        {
                                            abort();
                                        }
                                        rgb_pixel p;
                                        p.red   = red[buf[0]];
                                        p.green = green[buf[0]];
                                        p.blue  = blue[buf[0]];
                                        assign_pixel(image[row][col],p);

                                        ++col;
                                    }

                                    // if we read an uneven number of bytes then we need to read and
                                    // discard the next byte.
                                    if ((command&1) != 1)
                                    {
                                        if (in.sgetn(reinterpret_cast<char*>(buf),1) != 1)
                                        {
                                            abort();
                                        }
                                    }

                                    continue;
                                }

                                rgb_pixel p;

                                if (row < 0 || col + count > image.nc())
                                {
                                    // If this is just some padding bytes at the end then ignore them
                                    if (row >= 0 && col + count <= image.nc() + padding)
                                        continue;

                                    abort();
                                }

                                // put the bytes into the image
                                for (unsigned int i = 0; i < count; ++i)
                                {
                                    p.red   = red[command];
                                    p.green = green[command];
                                    p.blue  = blue[command];
                                    assign_pixel(image[row][col],p);

                                    ++col;
                                }
                            }
                        }



                    }
                    break;
                case 16:
                    abort();
                case 24:
                    {
                        // figure out how the pixels are packed
                        long padding;
                        if (bfSize - bfOffBits == biWidth*biHeight*3)
                            padding = 0;
                        else
                            padding = 4 - (biWidth*3)%4;

                        // check for this case.  It shouldn't happen but some BMP writers screw up the files
                        // so we have to do this.
                        if (biHeight*(biWidth*3+padding) > bfSize - bfOffBits)
                            padding = 0;
                        
                        // seek to the start of the pixel data
                        while (bytes_read_so_far != bfOffBits)
                        {
                            const long to_read = (long)std::min(bfOffBits - bytes_read_so_far, (unsigned long)sizeof(buf));
                            if (in.sgetn(reinterpret_cast<char*>(buf), to_read) != to_read)
                            {
                                abort();
                            }
                            bytes_read_so_far += to_read;
                        }

                        // load the image data
                        for (long row = biHeight-1; row >= 0; --row)
                        {
                            for (unsigned long col = 0; col < biWidth; ++col)
                            {
                                if (in.sgetn(reinterpret_cast<char*>(buf),3) != 3)
                                {
                                    abort();
                                }

                                rgb_pixel p;
                                p.red = buf[2];
                                p.green = buf[1];
                                p.blue = buf[0];
                                assign_pixel(image[row][col], p);

                            }
                            if (in.sgetn(reinterpret_cast<char*>(buf),padding) != padding)
                                abort();
                        }

                        break;
                    }
                case 32:
                    abort();
                default:
                    abort();

            }
        }
        catch (...)
        {
            image.clear();
            abort();
        }

    }

// ----------------------------------------------------------------------------------------

    template <
        typename image_type 
        >
    void load_dng (
        image_type& image_,
        std::istream& in
    )
    {
        image_view<image_type> image(image_);
        using namespace dng_helpers_namespace;
        try
        {
            if (in.get() != 'D' || in.get() != 'N' || in.get() != 'G')
                abort();

            unsigned long version;
            deserialize(version,in);
            if (version != 1)
                abort();

            unsigned long type;
            deserialize(type,in);

            long width, height;
            deserialize(width,in);
            deserialize(height,in);

            if (width > 0 && height > 0)
                image.set_size(height,width);
            else
                image.clear();

            if (type != grayscale_float)
            {
                typedef entropy_decoder::kernel_2a decoder_type;
                decoder_type decoder;
                decoder.set_stream(in);

                entropy_decoder_model<256,decoder_type>::kernel_5a edm(decoder);
                unsigned long symbol;
                rgb_pixel p_rgb;
                rgb_alpha_pixel p_rgba;
                hsi_pixel p_hsi;
                switch (type)
                {
                    case rgb_alpha_paeth:

                        for (long r = 0; r < image.nr(); ++r)
                        {
                            for (long c = 0; c < image.nc(); ++c)
                            {
                                p_rgba = predictor_rgb_alpha_paeth(image,r,c);
                                edm.decode(symbol);
                                p_rgba.red += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.green += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.blue += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.alpha += static_cast<unsigned char>(symbol);

                                assign_pixel(image[r][c],p_rgba);
                            }
                        }
                        break;

                    case rgb_alpha:

                        for (long r = 0; r < image.nr(); ++r)
                        {
                            for (long c = 0; c < image.nc(); ++c)
                            {
                                p_rgba = predictor_rgb_alpha(image,r,c);
                                edm.decode(symbol);
                                p_rgba.red += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.green += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.blue += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgba.alpha += static_cast<unsigned char>(symbol);

                                assign_pixel(image[r][c],p_rgba);
                            }
                        }
                        break;

                    case rgb_paeth:

                        for (long r = 0; r < image.nr(); ++r)
                        {
                            for (long c = 0; c < image.nc(); ++c)
                            {
                                p_rgb = predictor_rgb_paeth(image,r,c);
                                edm.decode(symbol);
                                p_rgb.red += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgb.green += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgb.blue += static_cast<unsigned char>(symbol);

                                assign_pixel(image[r][c],p_rgb);
                            }
                        }
                        break;

                    case rgb:

                        for (long r = 0; r < image.nr(); ++r)
                        {
                            for (long c = 0; c < image.nc(); ++c)
                            {
                                p_rgb = predictor_rgb(image,r,c);
                                edm.decode(symbol);
                                p_rgb.red += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgb.green += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_rgb.blue += static_cast<unsigned char>(symbol);

                                assign_pixel(image[r][c],p_rgb);
                            }
                        }
                        break;

                    case hsi:

                        for (long r = 0; r < image.nr(); ++r)
                        {
                            for (long c = 0; c < image.nc(); ++c)
                            {
                                p_hsi = predictor_hsi(image,r,c);
                                edm.decode(symbol);
                                p_hsi.h += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_hsi.s += static_cast<unsigned char>(symbol);

                                edm.decode(symbol);
                                p_hsi.i += static_cast<unsigned char>(symbol);

                                assign_pixel(image[r][c],p_hsi);
                            }
                        }
                        break;

                    case grayscale:
                        {
                            unsigned char p;
                            for (long r = 0; r < image.nr(); ++r)
                            {
                                for (long c = 0; c < image.nc(); ++c)
                                {
                                    edm.decode(symbol);
                                    p = static_cast<unsigned char>(symbol);
                                    p +=  predictor_grayscale(image,r,c);
                                    assign_pixel(image[r][c],p);
                                }
                            }
                        }
                        break;

                    case grayscale_16bit:
                        {
                            uint16 p;
                            for (long r = 0; r < image.nr(); ++r)
                            {
                                for (long c = 0; c < image.nc(); ++c)
                                {
                                    edm.decode(symbol);
                                    p = static_cast<uint16>(symbol);
                                    p <<= 8;
                                    edm.decode(symbol);
                                    p |= static_cast<uint16>(symbol);

                                    p +=  predictor_grayscale_16(image,r,c);
                                    assign_pixel(image[r][c],p);
                                }
                            }
                        }
                        break;

                    default:
                        abort();
                } // switch (type)

                edm.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
            }
            else // if this is a grayscale_float type image
            {
                std::vector<int64> man(image.size());
                std::vector<char> expbuf;
                // get the mantissa data
                for (unsigned long j = 0; j < man.size(); ++j)
                    deserialize(man[j], in);
                // get the compressed exponent data
                deserialize(expbuf, in);
                typedef entropy_decoder::kernel_2a decoder_type;
                typedef entropy_decoder_model<256,decoder_type>::kernel_4a edm_exp_type; 
                vectorstream inexp(expbuf);
                decoder_type decoder;
                decoder.set_stream(inexp);

                edm_exp_type edm_exp(decoder);
                float_details prev;
                unsigned long i = 0;
                // fill out the image 
                for (long r = 0; r < image.nr(); ++r)
                {
                    for (long c = 0; c < image.nc(); ++c)
                    {
                        unsigned long exp1, exp2;
                        edm_exp.decode(exp1);
                        edm_exp.decode(exp2);

                        float_details cur(man[i++],(exp2<<8) | exp1);
                        cur.exponent += prev.exponent;
                        cur.mantissa += prev.mantissa;
                        prev = cur;
                        
                        // Only use long double precision if the target image contains long
                        // doubles because it's slower to use those.
                        if (!is_same_type<typename image_traits<image_type>::pixel_type,long double>::value)
                        {
                            double temp = cur;
                            assign_pixel(image[r][c],temp);
                        }
                        else
                        {
                            long double temp = cur;
                            assign_pixel(image[r][c],temp);
                        }
                    }
                }
                unsigned long symbol;
                edm_exp.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm_exp.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm_exp.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
                edm_exp.decode(symbol);
                if (symbol != dng_magic_byte)
                    abort();
            }
        }
        catch (...)
        {
            image.clear();
            abort();
        }

    }

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    void load_bmp (
        image_type& image,
        const std::string& file_name
    )
    {
        std::ifstream fin(file_name.c_str(), std::ios::binary);
        if (!fin)
            abort();
        load_bmp(image, fin);
    }

// ----------------------------------------------------------------------------------------

    template <typename image_type>
    void load_dng (
        image_type& image,
        const std::string& file_name
    )
    {
        std::ifstream fin(file_name.c_str(), std::ios::binary);
        if (!fin)
            abort();
        load_dng(image, fin);
    }

// ----------------------------------------------------------------------------------------

}

#endif // DLIB_IMAGE_LOADEr_



