// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
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

#include <iostream>
#include <fstream>
#include <string>
#include <zxing/common/Counted.h>
#include <zxing/Binarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <exception>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>

#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/multi/ByQuadrantReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>

#include <opencv2/opencv.hpp>
#include "CVReaderSource.hpp"


using namespace std;
using namespace zxing;
using namespace zxing::multi;
using namespace zxing::qrcode;

namespace {
    
    bool more = false;
    bool test_mode = false;
    bool try_harder = false;
    bool search_multi = false;
    bool use_hybrid = false;
    bool use_global = false;
    bool verbose = false;
    
}

vector<Ref<Result> > decode(Ref<BinaryBitmap> image, DecodeHints hints) {
    Ref<Reader> reader(new MultiFormatReader);
    return vector<Ref<Result> >(1, reader->decode(image, hints));
}

vector<Ref<Result> > decode_multi(Ref<BinaryBitmap> image, DecodeHints hints) {
    MultiFormatReader delegate;
    GenericMultipleBarcodeReader reader(delegate);
    return reader.decodeMultiple(image, hints);
}

int read_image(Ref<LuminanceSource> source, bool hybrid, string expected) {
    vector<Ref<Result> > results;
    string cell_result;
    int res = -1;
    
    try {
        Ref<Binarizer> binarizer;
        if (hybrid) {
            binarizer = new HybridBinarizer(source);
        } else {
            binarizer = new GlobalHistogramBinarizer(source);
        }
        DecodeHints hints(DecodeHints::DEFAULT_HINT);
        hints.setTryHarder(try_harder);
        Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
        if (search_multi) {
            results = decode_multi(binary, hints);
        } else {
            results = decode(binary, hints);
        }
        res = 0;
    } catch (const ReaderException& e) {
        cell_result = "zxing::ReaderException: " + string(e.what());
        res = -2;
    } catch (const zxing::IllegalArgumentException& e) {
        cell_result = "zxing::IllegalArgumentException: " + string(e.what());
        res = -3;
    } catch (const zxing::Exception& e) {
        cell_result = "zxing::Exception: " + string(e.what());
        res = -4;
    } catch (const std::exception& e) {
        cell_result = "std::exception: " + string(e.what());
        res = -5;
    }
    
    if (test_mode && results.size() == 1) {
        std::string result = results[0]->getText()->getText();
        if (expected.empty()) {
            cout << "  Expected text or binary data for image missing." << endl
            << "  Detected: " << result << endl;
            res = -6;
        } else {
            if (expected.compare(result) != 0) {
//                cout << "  Expected: " << expected << endl
                cout 
                << "  Detected: " << result << endl;
                cell_result = "data did not match";
                res = -6;
            }
        }
    }
    
    if (res != 0 && (verbose || (use_global ^ use_hybrid))) {
//        cout << (hybrid ? "Hybrid" : "Global")
//        << " binarizer failed: " << cell_result << endl;
    } else if (!test_mode) {
        if (verbose) {
            cout << (hybrid ? "Hybrid" : "Global")
            << " binarizer succeeded: " << endl;
        }
        for (size_t i = 0; i < results.size(); i++) {
            if (more) {
                cout << "  Format: "
                << BarcodeFormat::barcodeFormatNames[results[i]->getBarcodeFormat()]
                << endl;
                for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
                    cout << "  Point[" << j <<  "]: "
                    << results[i]->getResultPoints()[j]->getX() << " "
                    << results[i]->getResultPoints()[j]->getY() << endl;
                }
            }
            if (verbose) {
                cout << "    ";
            }
            cout << results[i]->getText()->getText() << endl;
        }
    }
    
    return res;
}

string read_expected(string imagefilename) {
    string textfilename = imagefilename;
    string::size_type dotpos = textfilename.rfind(".");
    
    textfilename.replace(dotpos + 1, textfilename.length() - dotpos - 1, "txt");
    ifstream textfile(textfilename.c_str(), ios::binary);
    textfilename.replace(dotpos + 1, textfilename.length() - dotpos - 1, "bin");
    ifstream binfile(textfilename.c_str(), ios::binary);
    ifstream *file = 0;
    if (textfile.is_open()) {
        file = &textfile;
    } else if (binfile.is_open()) {
        file = &binfile;
    } else {
        return std::string();
    }
    file->seekg(0, ios_base::end);
    size_t size = size_t(file->tellg());
    file->seekg(0, ios_base::beg);
    
    if (size == 0) {
        return std::string();
    }
    
    char* data = new char[size + 1];
    file->read(data, size);
    data[size] = '\0';
    string expected(data);
    delete[] data;
    
    return expected;
}

int main(int argc, char** argv) {
    
    cv::VideoCapture cap(1);
    cv::Mat cameraView;
    test_mode = true;
    try_harder = true;
    use_hybrid = true;
    use_global = false;
    verbose = true;
    cap >> cameraView;
    cv::Rect rect(cameraView.cols / 3, cameraView.rows / 3, cameraView.cols / 3, cameraView.rows / 3);
    
    while(true){
        cap >> cameraView;
        
        cv::Mat roi = cameraView(rect);
        
        Ref<LuminanceSource> source;
        source = CVReaderSource::create(roi);
        
        read_image(source, true, string("test"));
        
        cv::rectangle(cameraView, rect, cv::Scalar(0, 0, 255));
        cv::imshow("zxing", cameraView);
        
        cv::waitKey(20);
    }
    
    return 0;
}
