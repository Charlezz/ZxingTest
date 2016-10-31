
#ifndef __CV_READER_SOURCE_H_
#define __CV_READER_SOURCE_H_
/*
 *  Copyright 2010-2011 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zxing/LuminanceSource.h>
#include <opencv2/opencv.hpp>

class CVReaderSource : public zxing::LuminanceSource {
private:
    typedef LuminanceSource Super;
    
    const zxing::ArrayRef<char> image;
    const int comps;
    
    char convertPixel(const char* pixel) const;
    
public:
    static zxing::Ref<LuminanceSource> create(cv::Mat mat);
    
    CVReaderSource(zxing::ArrayRef<char> image, int width, int height, int comps);
    
    zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
    zxing::ArrayRef<char> getMatrix() const;
};
#endif /* __CV_READER_SOURCE_H_ */
