/*==============================================================================

    ofxVisualProgramming: A visual programming patching environment for OF

    Copyright (c) 2022 Daan de Lange

    ofxVisualProgramming is distributed under the MIT License.
    This gives everyone the freedoms to use ofxVisualProgramming in any context:
    commercial or non-commercial, public or private, open or closed source.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    See https://github.com/d3cod3/ofxVisualProgramming for documentation

==============================================================================*/

#ifndef OFXVP_BUILD_WITH_MINIMAL_OBJECTS

#include "lossyJPEGCompression.h"
#include "ofxVpObjectsDaanHelpers.h"

// Pin map
#define PIN_OUT_PIXELS 0
#define PIN_IN_PIXELS 0
#define PIN_IN_QUALITY 1

//--------------------------------------------------------------
lossyJPEGCompression::lossyJPEGCompression() : PatchObject("lossy JPEG compression"){
    
    this->numInlets  = 2;
    this->numOutlets = 1;

    _inletParams[PIN_IN_PIXELS] = new ofPixels();  // input
    _inletParams[PIN_IN_QUALITY] = new float();  // input

    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_inletParams[PIN_IN_QUALITY]) = 0.0f;
    //*ofxVPD_GET_PIN_PTR_LVALUE(_outletParams[PIN_IN_PIXELS], ofPixels) = new ofPixels(); // output
    _outletParams[PIN_OUT_PIXELS] = new ofPixels(); // output
    //_outletParams[PIN_OUT_PIXELS] = &outPixels; // output

    this->initInletsState();

    this->bInitialised = false;
    this->bIsThreaded = false;
    this->fJpegQuality = 80.f;

    handleCompress = tjInitCompress();
    if (handleCompress == NULL){
        ofLogError("lossyJPEGCompression()") << "Error in tjInitCompress():\n%s\n", tjGetErrorStr();
    }

    handleDecompress = tjInitDecompress();
    if (handleDecompress == NULL){
        ofLogError("lossyJPEGCompression()") << "Error in tjInitDeCompress():\n%s\n", tjGetErrorStr();
    }

    this->setIsResizable(true);

}

//--------------------------------------------------------------
void lossyJPEGCompression::newObject(){
    PatchObject::setName( this->objectName );

    this->addInlet(VP_LINK_PIXELS,"pixelsToCompress");
    this->addInlet(VP_LINK_NUMERIC,"jpegQuality");

    this->addOutlet(VP_LINK_PIXELS,"compressedPixels");

}

//--------------------------------------------------------------
void lossyJPEGCompression::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    this->fJpegQuality = this->getCustomVar("JPEG-quality");
}

//--------------------------------------------------------------
void lossyJPEGCompression::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){

    // UPDATE STUFF
    // Read quality from inlet
    if( this->inletsConnected[PIN_IN_QUALITY] ){
        //float inQuality = *(float *)&_inletParams[1];
        float* inQuality = ofxVPD_GET_PIN_PTR_LVALUE<float>(_inletParams[PIN_IN_QUALITY]);
//        float* inQuality = reinterpret_cast<float*>(&(_inletParams[1]));
        // Valid image ?
        if(inQuality){
            this->fJpegQuality = *inQuality;
            std::cout << "In quality = " << this->fJpegQuality << " (" << _inletParams[1] << "/" << &_inletParams[1] << ") " << std::endl;
        }
    }
    // maybe ok ?
//    if( this->inletsConnected[1] && _inletParams[1] ){
//        //float inQuality = *(float *)&_inletParams[1];
//        //float inQuality = *static_cast<float*>(_inletParams[1]);
//        float* inQuality = static_cast<float*>((_inletParams[1]));
//        // Valid image ?
//        if(inQuality){
//            this->fJpegQuality = *inQuality;
//            std::cout << "In quality = " << this->fJpegQuality << " (" << &_inletParams[1] << ") " << std::endl;
//        }
//    }

    // Read pixels from inlet [or should this be done in drawObjectContent() ?]
    // Connected ?
    if( this->inletsConnected[PIN_IN_PIXELS] ){
        //ofPixels* pixels = static_cast<ofPixels *>(_inletParams[0]);
        ofPixels* pixels = ofxVPD_GET_PIN_PTR_LVALUE<ofPixels>(_inletParams[PIN_IN_PIXELS]);
        // Valid image ?
        if( pixels && pixels->isAllocated() ){
            outPixels = *pixels; // Duplication needed not to edit the connected original
            threadablePixelsFunction(outPixels); // Note: if this fails, a copy of the original pixels object is passed to ensure the pixels remain available and valid. (Is this the desired behaviour ?)
//            threadablePixelsFunction(*pixels);
            // MO original: *static_cast<ofTexture *>(_outletParams[0]);
            // Bis: static_cast<ofTexture *>(_inletParams[0])->readToPixels(*static_cast<ofPixels *>(_outletParams[0]));
            _outletParams[PIN_OUT_PIXELS] = &outPixels;
            //*ofxVPD_GET_PIN_PTR_LVALUE<ofPixels>(_outletParams[PIN_OUT_PIXELS]) = &outPixels;
        }
        else {
            ofLogError("lossyJPEGCompression::updateObjectContent()") << "No valid pixels in connected link ! :o" << std::endl;
        }
    }

}

//--------------------------------------------------------------
void lossyJPEGCompression::drawObjectContent(ofTrueTypeFont *font, shared_ptr<ofBaseGLRenderer>& glRenderer){

}

//--------------------------------------------------------------
void lossyJPEGCompression::drawObjectNodeGui( ImGuiEx::NodeCanvas& _nodeCanvas ){

    // CONFIG GUI inside Menu
    if(_nodeCanvas.BeginNodeMenu()){

        ImGui::Separator();
        ImGui::Separator();
        ImGui::Separator();

        if (ImGui::BeginMenu("CONFIG"))
        {

            drawObjectNodeConfig(); this->configMenuWidth = ImGui::GetWindowWidth();

            ImGui::EndMenu();
        }

        _nodeCanvas.EndNodeMenu();
    }

    // Visualize (Object main view)
    if( _nodeCanvas.BeginNodeContent(ImGuiExNodeView_Visualise) ){
        // Show output image ?
        ofPixels* pixels = ofxVPD_GET_PIN_PTR_LVALUE<ofPixels>(_outletParams[PIN_OUT_PIXELS]);
        if(pixels && pixels->isAllocated()){
//            const bool isUsingArb = ofGetUsingArbTex();
//            if (isUsingArb){
//                ofDisableArbTex();
//            }
            ofTexture tex;
//            tex.getTextureData().textureTarget = GL_TEXTURE_2D;
//            tex.loadData(*pixels);
//            if (isUsingArb){
//                ofEnableArbTex();
//            }
            tex.allocate(*pixels, false);
            tex.loadData(*pixels);

            ImGui::Text("Output pixels %.0fx%.0f", tex.getWidth(), tex.getHeight());
            if(tex.isAllocated()){
                ImGui::Image(GetImTextureID(tex), ImVec2(100,100));
            }
            else {
                ImGui::TextDisabled("No out texture");
            }
        }
        else {
            ImGui::TextDisabled("No out pixels !");
        }

//        ofPixels* pixelsIn = ofxVPD_GET_PIN_PTR_LVALUE<ofPixels>(_inletParams[PIN_IN_PIXELS]);
//        if(pixelsIn && pixelsIn->isAllocated()){
//            ofTexture tex;
//            tex.loadData(*pixelsIn);
//            ImGui::Text("Input pixels");
//            if(tex.isAllocated()){
//                ImGui::Image(GetImTextureID(tex), ImVec2(100,100));
//            }
//            else {
//                ImGui::TextDisabled("No in texture");
//            }
//        }
//        else {
//            ImGui::TextDisabled("No input pixels !");
//        }
        _nodeCanvas.EndNodeContent();
    }
}

//--------------------------------------------------------------
void lossyJPEGCompression::drawObjectNodeConfig(){
    ImGui::Spacing();

    if( ImGui::SliderFloat("JPEG Quality", &this->fJpegQuality, 0.f, 100.f, "%.0f") ){
        this->setCustomVar(fJpegQuality,"JPEG-quality");
    }

    ImGuiEx::ObjectInfo(
                "Encode images in JPEG and read them back (no files, uses a buffer).",
                "https://mosaic.d3cod3.org/reference.php?r=lossy-jpeg-compression", scaleFactor);
}

//--------------------------------------------------------------
void lossyJPEGCompression::removeObjectContent(bool removeFileFromData){
    // Close threads and mutexes
    this->stopTurboThread();

    toCompress.close();
    receiveCompressed.close();
    waitForThread(true, 1000);

    // Release turboJpeg handles
    if (handleCompress){
        tjDestroy(handleCompress);
    }
    if (handleDecompress){
        tjDestroy(handleDecompress);
    }
    handleCompress = NULL;
    handleDecompress = NULL;
}

// Stops threads
void lossyJPEGCompression::stopTurboThread(){
	if( this->isThreadRunning() ){
		this->stopThread();
	}
}

// Starts threads
void lossyJPEGCompression::startTurboThread(const bool restart){
	if( !restart){
		if( !this->isThreadRunning() ){
			this->startThread();
		}
	}
	else {
		this->stopTurboThread();
		//waitForThread();

		this->startThread();
	}

}

void lossyJPEGCompression::threadedFunction(){
	// wait until there's a new frame
	// this blocks the thread, so it doesn't use
	// the CPU at all, until a frame arrives.
	// also receive doesn't allocate or make any copies
	ofPixels pixels;

	while(toCompress.receive(pixels)){
		this->threadablePixelsFunction(pixels);

		// once processed send the result back to the main thread. in c++11 we can move it to avoid a copy
		//receiveCompressed.empty();
#if __cplusplus>=201103
		receiveCompressed.send(std::move(pixels));
#else
		receiveCompressed.send(pixels);
#endif
	}
}

void lossyJPEGCompression::threadablePixelsFunction( ofPixels& pixels){
	// Fixme: remove error logging if it's threaded !

	// don't process empty images
	if(!pixels.isAllocated() || this->handleCompress == NULL || this->handleDecompress == NULL || pixels.getWidth()==0 || pixels.getHeight()==0){
        std::cout << "Ignoring empty pixels object!" << std::endl;
		return;
	}

	// COMPRESS IMAGE to JPEG
	//ofBuffer buffer;
	int bpp = pixels.getBytesPerPixel();
	int w = pixels.getWidth();
	int h = pixels.getHeight();
	ofBuffer buffer;

	int jpegQuality = glm::clamp((this->fJpegQuality+1.f)*50.f, 0.f, 100.f);
	//cout << "encoding with Quality = " << jpegQuality << " // " << this->fJpegQuality << endl;

	int pitch = (w*bpp);
	int flags = 0;
	unsigned long size = w*h*bpp;

	int jpegsubsamp = ofNoise(ofGetElapsedTimef()*10.0)*4.0;

	if ( pixels.getImageType() == OF_IMAGE_COLOR_ALPHA || pixels.getImageType() == OF_IMAGE_COLOR){
		vector<unsigned char> buf; // guaranteed to be destroyed
		buf.resize(size);

		unsigned char * output = &buf[0];
		bool compressOK = false;

		try {
			int result = -1; // tmp
			result = tjCompress(handleCompress, (unsigned char*)(pixels.getData()), w, pitch, h, bpp, output, &size, jpegsubsamp, jpegQuality, flags);

			if(result==0){
				buffer.set((const char*)output, size);
			}

			//std::cout << "TurboJPEG quality=" << jpegQuality << "\t" << jpegsubsamp << "\t" << flags << "\t" << result << "\t" << buf.size() << std::endl;
			compressOK = (result==0);
		}
		catch(std::exception& _e){
			compressOK = false;
			// todo: handle error
		}

		if(compressOK){
			// DECOMPRESS IMAGE
			int subsamp;
			int ok = 0;
			try {
				//ok = tjDecompressHeader2(handleDecompress, (unsigned char*)buffer.getData(), buffer.size(), &w, &h, &subsamp);
				ok = tjDecompressHeader2(handleDecompress, (unsigned char*) output, size, &w, &h, &subsamp);
			}
			catch(std::exception& _e){
				// todo: handle error
				ofLogError("lossyJPEGCompression::threadablePixelsFunction()") << "Error decompressing image..." << std::endl;
			}
			//cout << subsamp << endl;
			if (ok != 0)
			{
				ofLogError("lossyJPEGCompression::threadablePixelsFunction()") << "Error in tjDecompressHeader2():\n%s\n", tjGetErrorStr();
				//cout << jpegQuality << "\t" << jpegsubsamp << "\t" << flags << "\t" << " " << endl;
				return;
			}

			tjDecompress(handleDecompress, (unsigned char*)buffer.getData(), buffer.size(), pixels.getData(), w, 0, h, bpp, 0);

			// done ! :)
		}
		else {
			ofLogError("lossyJPEGCompression::threadablePixelsFunction()") << "Error compressing pixels !" << std::endl;
		}
	}
    else {
        ofLogError("lossyJPEGCompression::threadablePixelsFunction()") << "No correct image !" << std::endl;
    }
}


OBJECT_REGISTER( lossyJPEGCompression, "lossy JPEG compression", OFXVP_OBJECT_CAT_TEXTURE)

#endif
