#ifndef SZ_GENERAL_COMPRESSOR_HPP
#define SZ_GENERAL_COMPRESSOR_HPP

#include "compressor/Compressor.hpp"
#include "frontend/Frontend.hpp"
#include "encoder/Encoder.hpp"
#include "lossless/Lossless.hpp"
#include "utils/FileUtil.h"
#include "utils/Config.hpp"
#include "def.hpp"
#include <cstring>

namespace SZ {
    template<class T, uint N, class Frontend, class Encoder, class Lossless>
    class SZGeneralCompressor : public concepts::CompressorInterface<T> {
    public:


        SZGeneralCompressor(const Config<T, N> &conf,
                            Frontend frontend, Encoder encoder, Lossless lossless) :
                frontend(frontend), encoder(encoder), lossless(lossless) {
            static_assert(std::is_base_of<concepts::FrontendInterface<T, N>, Frontend>::value,
                          "must implement the frontend interface");
            static_assert(std::is_base_of<concepts::EncoderInterface<int>, Encoder>::value,
                          "must implement the encoder interface");
            static_assert(std::is_base_of<concepts::LosslessInterface, Lossless>::value,
                          "must implement the lossless interface");
        }

        uchar *compress(T *data, size_t &compressed_size) {

            std::vector<int> quant_inds = frontend.compress(data);

            uchar *compressed_data = new uchar[2 * quant_inds.size() * sizeof(T)];
            uchar *compressed_data_pos = compressed_data;

            frontend.save(compressed_data_pos);

            encoder.preprocess_encode(quant_inds, 2 * frontend.get_radius());
            encoder.save(compressed_data_pos);
            encoder.encode(quant_inds, compressed_data_pos);
            encoder.postprocess_encode();

            uchar *lossless_data = lossless.compress(compressed_data,
                                                     compressed_data_pos - compressed_data,
                                                     compressed_size);
            lossless.postcompress_data(compressed_data);
            return lossless_data;
        }

        T *decompress(uchar const *lossless_compressed_data, const size_t length) {
            size_t remaining_length = length;
            auto compressed_data = lossless.decompress(lossless_compressed_data, remaining_length);
            uchar const *compressed_data_pos = compressed_data;

            frontend.load(compressed_data_pos, remaining_length);

            encoder.load(compressed_data_pos, remaining_length);

            auto quant_inds = encoder.decode(compressed_data_pos, frontend.get_num_elements());
            encoder.postprocess_decode();

            lossless.postdecompress_data(compressed_data);

            return frontend.decompress(quant_inds);
        }


    private:
        Frontend frontend;
        Encoder encoder;
        Lossless lossless;
    };

    template<class T, uint N, class Frontend, class Encoder, class Lossless>
    SZGeneralCompressor<T, N, Frontend, Encoder, Lossless>
    make_sz_general_compressor(const Config<T, N> &conf, Frontend frontend, Encoder encoder, Lossless lossless) {
        return SZGeneralCompressor<T, N, Frontend, Encoder, Lossless>(conf, frontend, encoder, lossless);
    }
}
#endif
