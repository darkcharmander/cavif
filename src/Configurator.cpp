//
// Created by psi on 2020/01/14.
//

#include <iostream>

#include "Configurator.hpp"

namespace {

std::string basename(std::string const& path) {
  auto pos = path.find_last_of('/');
  if(pos == std::string::npos) {
    return path;
  }
  return path.substr(pos+1);
}

}

int Configurator::parse(int argc, char **argv) {
  using namespace clipp;
  auto& aom = this->encoderConfig;
  auto cli = (
      required("-i", "--input") & value("input.{png, bmp}", input),
      required("-o", "--output") & value("output.avif", output),
      option("--monochrome").doc("Encode to monochrome image.").set(encoderConfig.monochrome, 1u),
      option("--usage").doc("Encoder usage") & (parameter("good").doc("Good Quality mode").set(aom.g_usage, static_cast<unsigned int>(AOM_USAGE_GOOD_QUALITY)) | parameter("realtime").doc("Real time encoding mode.").set(aom.g_usage, static_cast<unsigned int>(AOM_USAGE_REALTIME))),
      option("--threads") & integer("Num of threads to use", aom.g_threads),
      option("--profile") & integer("AV1 Profile(0=base, 1=high, 2=professional)", aom.g_profile),
      option("--bit-depth").doc("Bit depth of output image.") & (parameter("8").set(aom.g_bit_depth, AOM_BITS_8) | parameter("10").set(aom.g_bit_depth, AOM_BITS_10) | parameter("12").set(aom.g_bit_depth, AOM_BITS_10)),
      option("--rate-control").doc("Rate control method") & (parameter("q").doc("Constant Quality").set(aom.rc_end_usage, AOM_Q) | parameter("cq").doc("Constrained Quality").set(aom.rc_end_usage, AOM_CQ)),
      option("--large-scale-tile").doc("Use large scale tile mode.").set(aom.large_scale_tile, 1u),
      option("--full-still-picture-header").doc("Force to output full picture header").set(aom.full_still_picture_hdr, 1u),
      option("--enable-full-color-range").doc("Quality/Speed ratio modifier").set(fullColorRange),
      option("--crf") & integer("CQ Level in CQ rate control mode", crf),
      option("--cpu-used") & integer("Quality/Speed ratio modifier", cpu_used),
      option("--enable-cdef").doc("Quality/Speed ratio modifier").set(enableCDEF),
      option("--superblock-size").doc("Superblock size.") & (parameter("dynamic").doc("encoder determines the size automatically.").set(superblockSize, AOM_SUPERBLOCK_SIZE_DYNAMIC) | parameter("128").doc("use 128x128 superblock.").set(superblockSize, AOM_SUPERBLOCK_SIZE_128X128) | parameter("64").doc("use 64x64 superblock.").set(superblockSize, AOM_SUPERBLOCK_SIZE_64X64)),
      option("--tune").doc("Quality metric to tune") & (parameter("ssim").doc("structural similarity").set(tune, AOM_TUNE_SSIM) | parameter("psnr").doc("peak signal-to-noise ratio").set(tune, AOM_TUNE_PSNR) | parameter("cdef-dist").doc("cdef-dist").set(tune, AOM_TUNE_CDEF_DIST) | parameter("daala-dist").doc("daala-dist").set(tune, AOM_TUNE_DAALA_DIST))
  );
  if(!clipp::parse(argc, argv, cli)) {
    std::cerr << make_man_page(cli, basename(std::string(argv[0])));
    return -1;
  }
  if(input == output) {
    std::cerr << make_man_page(cli, basename(std::string(argv[0])));
    return -1;
  }
  return 0;
}

void Configurator::modify(aom_codec_ctx_t *codec) {
  //aom_codec_control(codec, AV1E_SET_DENOISE_NOISE_LEVEL, 1);
  aom_codec_control(codec, AOME_SET_CPUUSED, this->cpu_used);
  aom_codec_control(codec, AOME_SET_STATIC_THRESHOLD, 0);
  aom_codec_control(codec, AOME_SET_TUNING, tune);
  aom_codec_control(codec, AOME_SET_CQ_LEVEL, this->crf);
  aom_codec_control(codec, AV1E_SET_ENABLE_CDEF, enableCDEF ? 1 : 0);

  //FIXME(ledyba-z): support color profile. PNG can contain gamma correction and color profile.
  // Gamma Correction and Precision Color (PNG: The Definitive Guide)
  // http://www.libpng.org/pub/png/book/chapter10.html
  aom_codec_control(codec, AV1E_SET_COLOR_PRIMARIES,2 );
  aom_codec_control(codec, AV1E_SET_MATRIX_COEFFICIENTS,2 );
  aom_codec_control(codec, AV1E_SET_TRANSFER_CHARACTERISTICS, 2);
  //
  aom_codec_control(codec, AV1E_SET_COLOR_RANGE, fullColorRange ? 1 : 0);
  aom_codec_control(codec, AV1E_SET_SUPERBLOCK_SIZE, superblockSize);
}