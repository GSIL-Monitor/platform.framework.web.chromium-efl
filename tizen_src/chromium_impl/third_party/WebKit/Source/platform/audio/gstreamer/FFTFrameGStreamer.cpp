/*
 *  Copyright (C) 2012 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// FFTFrame implementation using the GStreamer FFT library.

#include "build/build_config.h"

#if defined(WEBAUDIO_GSTREAMER)

#include "platform/audio/FFTFrame.h"

#include "platform/audio/VectorMath.h"
#include "platform/wtf/StdLibExtras.h"

namespace {

size_t unpackedFFTDataSize(unsigned fftSize) {
  return fftSize / 2 + 1;
}

} // anonymous namespace

namespace blink {

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fft_size)
    : fft_size_(fft_size),
      log2fft_size_(static_cast<unsigned>(log2(fft_size))),
      real_data_(unpackedFFTDataSize(fft_size)),
      imag_data_(unpackedFFTDataSize(fft_size)) {
  complex_data_ = new GstFFTF32Complex[unpackedFFTDataSize(fft_size_)];

  int fftLength = gst_fft_next_fast_length(fft_size_);
  fft_ = gst_fft_f32_new(fftLength, FALSE);
  inverse_fft_ = gst_fft_f32_new(fftLength, TRUE);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame() : fft_size_(0), log2fft_size_(0), complex_data_(nullptr) {
  int fftLength = gst_fft_next_fast_length(fft_size_);
  fft_ = gst_fft_f32_new(fftLength, FALSE);
  inverse_fft_ = gst_fft_f32_new(fftLength, TRUE);
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : fft_size_(frame.fft_size_),
      log2fft_size_(frame.log2fft_size_),
      real_data_(frame.fft_size_ / 2),
      imag_data_(frame.fft_size_ / 2) {
  complex_data_ = new GstFFTF32Complex[unpackedFFTDataSize(fft_size_)];

  int fftLength = gst_fft_next_fast_length(fft_size_);
  fft_ = gst_fft_f32_new(fftLength, FALSE);
  inverse_fft_ = gst_fft_f32_new(fftLength, TRUE);

  // Copy/setup frame data.
  memcpy(RealData(), frame.RealData(),
         sizeof(float) * unpackedFFTDataSize(fft_size_));
  memcpy(ImagData(), frame.ImagData(),
         sizeof(float) * unpackedFFTDataSize(fft_size_));
}

void FFTFrame::Initialize() {}

void FFTFrame::Cleanup() {}

FFTFrame::~FFTFrame() {
  if (fft_)
    gst_fft_f32_free(fft_);

  if (inverse_fft_)
    gst_fft_f32_free(inverse_fft_);

  if (complex_data_)
    delete[](complex_data_);
}

void FFTFrame::DoFFT(const float* data) {
  gst_fft_f32_fft(fft_, data, complex_data_);

  // Scale the frequency domain data to match vecLib's scale factor
  // on the Mac. FIXME: if we change the definition of FFTFrame to
  // eliminate this scale factor then this code will need to change.
  // Also, if this loop turns out to be hot then we should use SSE
  // or other intrinsics to accelerate it.
  float scaleFactor = 2;

  float* imagData = ImagData();
  float* realData = RealData();
  for (unsigned i = 0; i < unpackedFFTDataSize(fft_size_); ++i) {
    imagData[i] = complex_data_[i].i * scaleFactor;
    realData[i] = complex_data_[i].r * scaleFactor;
  }
}

void FFTFrame::DoInverseFFT(float* data) {
  //  Merge the real and imaginary vectors to complex vector.
  float* realData = RealData();
  float* imagData = ImagData();

  for (size_t i = 0; i < unpackedFFTDataSize(fft_size_); ++i) {
    complex_data_[i].i = imagData[i];
    complex_data_[i].r = realData[i];
  }

  gst_fft_f32_inverse_fft(inverse_fft_, complex_data_, data);

  // Scale so that a forward then inverse FFT yields exactly the original data.
  const float scaleFactor = 1.0 / (2 * fft_size_);
  VectorMath::Vsmul(data, 1, &scaleFactor, data, 1, fft_size_);
}

} // namespace blink

#endif // USE(WEBAUDIO_GSTREAMER)
