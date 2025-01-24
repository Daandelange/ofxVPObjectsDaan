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

#include "ballInABox.h"
#include "ofxVpObjectsDaanHelpers.h"

// Pin mappings
#define PIN_IN_SPEED 0
#define PIN_IN_VELX 1
#define PIN_IN_VELY 2
#define PIN_IN_RAND 3
#define PIN_OUT_BANG 0
#define PIN_OUT_POSX 1
#define PIN_OUT_POSY 2

//--------------------------------------------------------------
BallInABox::BallInABox() :
            PatchObject("Ball in a box")
{

    this->numInlets  = 4;
    this->numOutlets = 3;

    outBangFloat = 0;
    outX=0;
    outY=0;

    // Bind outputs
    _inletParams[PIN_IN_SPEED] = new float();
    _inletParams[PIN_IN_VELX] = new float();
    _inletParams[PIN_IN_VELY] = new float();
    _inletParams[PIN_IN_RAND] = new float();

    _outletParams[PIN_OUT_BANG] = &outBangFloat;
    _outletParams[PIN_OUT_POSX] = &outX;//&pos.x;
    _outletParams[PIN_OUT_POSY] = &outY;//&pos.y;
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_BANG]) = outBangFloat;
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_POSX]) = outX;//&pos.x;
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_POSY]) = outY;//&pos.y;

    this->initInletsState();

    this->setIsResizable(true);

}

//--------------------------------------------------------------
void BallInABox::newObject(){
    PatchObject::setName( this->objectName );
    
    // Start simulation
    this->reset();

    this->addInlet(VP_LINK_NUMERIC,"Speed");
    this->addInlet(VP_LINK_NUMERIC,"Velocity X");
    this->addInlet(VP_LINK_NUMERIC,"Velocity Y");
    this->addInlet(VP_LINK_NUMERIC,"Randomize");
    
    this->addOutlet(VP_LINK_NUMERIC,"OutBang");
    this->addOutlet(VP_LINK_NUMERIC,"Position X");
    this->addOutlet(VP_LINK_NUMERIC,"Position Y");
}

//--------------------------------------------------------------
void BallInABox::setupObjectContent(shared_ptr<ofAppGLFWWindow> &mainWindow){
    unusedArgs(mainWindow);

    // Make box as height as width
    this->height = this->width - 20;
}

//--------------------------------------------------------------
void BallInABox::updateObjectContent(map<int,shared_ptr<PatchObject>> &patchObjects){
    unusedArgs(patchObjects);

    // Restrict box size
    //this->height = this->width + 20;

    // Update inlets
    if(this->inletsConnected[PIN_IN_SPEED]){
        speed = *ofxVPD_GET_PIN_PTR_LVALUE<float>(this->_inletParams[PIN_IN_SPEED]);
    }
    bool updatedVelocity = false;
    if(this->inletsConnected[PIN_IN_VELX]){
        vel.x = *ofxVPD_GET_PIN_PTR_LVALUE<float>(this->_inletParams[PIN_IN_VELX]);
        updatedVelocity = true;
    }
    if(this->inletsConnected[PIN_IN_VELY]){
        vel.x = *ofxVPD_GET_PIN_PTR_LVALUE<float>(this->_inletParams[PIN_IN_VELY]);
        updatedVelocity = true;
    }
    if(updatedVelocity){
        vel = glm::normalize(glm::vec2(ofRandomf(),ofRandomf()));
    }
    if(this->inletsConnected[PIN_IN_RAND]){
        if(1.f <= *ofxVPD_GET_PIN_PTR_LVALUE<float>(this->_inletParams[PIN_IN_RAND])){
            randomize();
        }
    }

    // Tick simulation
    if(!paused && speed>0){
        this->tickSimulation();
    }

    // Update out connections
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_BANG]) = outBangFloat;
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_POSX]) = ofMap(outX, -1.f+(ballSize), 1.f-(ballSize), 0.f, 1.f, true);
    *ofxVPD_GET_PIN_PTR_LVALUE<float>(_outletParams[PIN_OUT_POSY]) = ofMap(outY, -1.f+(ballSize), 1.f-(ballSize), 0.f, 1.f, true);
}

//--------------------------------------------------------------
void BallInABox::drawObjectContent(ofTrueTypeFont *font, shared_ptr<ofBaseGLRenderer>& glRenderer){
    unusedArgs(font,glRenderer);
}

//--------------------------------------------------------------
void BallInABox::drawObjectNodeGui( ImGuiEx::NodeCanvas& _nodeCanvas ){

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

        // Calc stuff
        ImVec2 window_pos = ImGui::GetWindowPos();
        const ImRect& nodeZone = _nodeCanvas.GetNodeData().innerContentBox;
        ImRect zone(nodeZone.GetTL(), nodeZone.GetBR());
        zone.Max.x -= 20; // removes padding, dirty
        zone.Max.y = zone.Min.y+zone.GetSize().x;// tmp y=x (square ratio)
        ImVec2 halfZoneSize = zone.GetSize()*ImVec2(.5, .5);
        ImVec2 imRectTL = window_pos+ImGui::GetCursorPos();
        ImVec2 imBallPos = imRectTL + halfZoneSize + ImVec2(pos.x*halfZoneSize.x,pos.y*halfZoneSize.y);
        
        auto* dl = ImGui::GetWindowDrawList();
        if(dl){
            // Reserves space
            ImGui::Dummy(halfZoneSize*ImVec2(2,2));
            // debug: zone
            //ImGui::GetForegroundDrawList()->AddRect(zone.Min, zone.Max, IM_COL32(200,255,200, 255));
            // Bang bg fill
            if(outBang && doFlash){
                dl->AddRectFilled(imRectTL, imRectTL+zone.GetSize(), IM_COL32(250, 250, 5, 255));
            }
            // Bounding box
            dl->AddRect(imRectTL, imRectTL+zone.GetSize(), IM_COL32(200,200,200, 255));
            // The ball
            dl->AddCircleFilled(imBallPos, ballSize*(halfZoneSize.x), IM_COL32(outBang?255:0, outBang?255:0, outBang?255:0, 255));
            // Velocity+Speed
            ImVec2 ballVecToPos = imBallPos + ((vel*halfZoneSize*(speed==0.f?.25f:speed)));
            dl->AddLine(imBallPos, ballVecToPos, IM_COL32(0,0,255,255));
        }

        _nodeCanvas.EndNodeContent();
    }

    // get imgui canvas zoom
    //canvasZoom = _nodeCanvas.GetCanvasScale();

}

//--------------------------------------------------------------
void BallInABox::drawObjectNodeConfig(){
    ImGui::DragFloat("Speed", &speed, 0.005f, 0.f, 10.f);
    ImGui::DragFloat("Position", &pos[0], 0.005f, -1.f, 1.f);
    ImGui::DragFloat("Size", &ballSize, 0.005f, 0.001f, 1.f);
    bool checkVelocity = ImGui::DragFloat2("Velocity", &vel[0], 0.5f, 0, 1, "%.3f");
    float angle = glm::sin(vel.x)+glm::cos(vel.y);
    if(ImGui::SliderAngle("VelocityAngle", &angle, 0.f, 360.f, "%.03f deg")){
        vel.x = glm::sin(angle);
        vel.y = glm::cos(angle);
        checkVelocity = true;
    }
    if(checkVelocity){
        vel = glm::normalize(vel);
    }

    if(ImGui::Button(paused ? "Resume":"Pause")){
        togglePause();
    }

    if(ImGui::Button("Randomize")){
        this->reset();
    }

    ImGui::Checkbox("Flash GUI (on bounce)", &doFlash);
    
    // Reset ?
    if(ImGui::Button("Reset")){
        this->reset();
    }

    if(ImGui::TreeNode("Internals")){
        ImGui::Text("Pos: [%.3f, %.3f]", pos.x, pos.y);
        ImGui::Text("Velocity: [%.3f, %.3f]", vel.x, vel.y);
        ImGui::Text("Bang: %s", outBang>=1?"1":"0");
        const float elapsed = lastFrameTime-startTime;
        ImGui::Text("Elapsed: %03.2f sec", elapsed);
        ImGui::Text("Ticks: %lu", numTicks);

        ImGui::TreePop();
    }

    
    ImGuiEx::ObjectInfo(
                "Simulates a ball bouncing in a box, emitting bangs every time it hits an edge.",
                "https://mosaic.d3cod3.org/reference.php?r=BallInABox", scaleFactor);
}

//--------------------------------------------------------------
void BallInABox::removeObjectContent(bool removeFileFromData){
    unusedArgs(removeFileFromData);
}

//--------------------------------------------------------------
void BallInABox::tickSimulation(){
    // Note: no security, internal use only !

    // Update elapsedTime
    const float now = ofGetElapsedTimef();
    const float elapsedTime = now-startTime;
    const float deltaTime = glm::max(0.f, elapsedTime-lastFrameTime);
    //std::cout << "DeltaTime=" << deltaTime << ", lastFrameTime=" << lastFrameTime << ", elapsedTime=" << elapsedTime << ", startTime="<< startTime << ", now=" << now << std::endl;

    // Tick simulation
    const float scaledSpeed = speed*2.f*deltaTime; // Note: Scale pixels to inner range (2=1/2 bcoz [-1->1]=2 range)
    glm::vec2 nextPos = pos + (vel*scaledSpeed);
    bool inverted = false;
    // X axis bounds
    if(nextPos.x >= 1-ballSize || nextPos.x <= -1+ballSize){
        // Invert velocity (takes 1 tick)
        vel.x *= -1;
        inverted = true;
        //while(nextPos.y<-1+ballSize*.5f || nextPos.y>1-ballSize*.5f)
            nextPos.x += vel.x*scaledSpeed;
    }
    // Move as normal
    else {
        pos.x = nextPos.x;
    }
    // Y axis bounds
    if(nextPos.y >= 1-ballSize || nextPos.y <= -1+ballSize){
        // Invert velocity (takes 1 tick)
        vel.y *= -1;
        inverted = true;
        //while(nextPos.y<-1+ballSize*.5f || nextPos.y>1-ballSize*.5f)
            nextPos.y += vel.y*scaledSpeed;
    }
    // Move as normal
    else {
        pos.y = nextPos.y;
    }

    // notify ?
    outBang = inverted;
    outBangFloat = outBang ? 1.f : 0.f;

    outX = pos.x;
    outY = pos.y;

    // Increment
    numTicks++;
    lastFrameTime = elapsedTime;
}

//--------------------------------------------------------------
void BallInABox::reset(){
    startTime = ofGetElapsedTimef();
    lastFrameTime = startTime;
    numTicks = 0;
    
    randomize();
}

//--------------------------------------------------------------
void BallInABox::randomize(){
    pos = {ofRandomf()*(1.f-ballSize),ofRandomf()*(1.f-ballSize)};
    //vel = glm::normalize(glm::vec2(ofRandomf()>0?-1:1,ofRandomf()>0?-1:1));
    vel = glm::normalize(glm::vec2(ofRandomf(),ofRandomf()));
}

//--------------------------------------------------------------
void BallInABox::togglePause(){
    if(paused){
        // Update elapsed time before resume
        const float now = ofGetElapsedTimef();
        const float elapsedTime = now-startTime;
        const float pausedTime = elapsedTime-lastFrameTime;
        std::cout << "PausedTime=" << pausedTime << ", elapsedTime=" << elapsedTime << ", " << std::endl;
//        lastFrameTime = now-lastFrameTime;
//        startTime += pausedTime;
        lastFrameTime += pausedTime;
        startTime += pausedTime;
    }
    paused = !paused;
}

OBJECT_REGISTER( BallInABox, "Ball in a box", OFXVP_OBJECT_CAT_LOGIC)

#endif
