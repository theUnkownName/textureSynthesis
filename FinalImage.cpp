#include "FinalImage.h"


typedef Graph<int,int,int> GraphType;

struct findRepeatedPatch
{
    double error;
    findRepeatedPatch(double error) : error(error) {}
    bool operator() (const Patch& m) const
    {
        return m.error == error;
    }
};

//Constructor
FinalImage::FinalImage(Mat &img, int y_expand, int x_expand, int windowSize)
{
    newimg = Mat::zeros(img.rows + y_expand  , img.cols + x_expand, CV_64FC3);
    newimg.convertTo(newimg, CV_8UC1);
    width = newimg.cols;
    height = newimg.rows;
    backgroundPorcentageTmp = 0;
    cout << " ------------Output image created--------------" << endl;
}

Mat FinalImage::graph_Cut(Mat& A, Mat& B, int overlap, int orientation)
{



    assert(A.data);
    assert(B.data);
    if (orientation == 1)
        assert(A.rows == B.rows);
    else if (orientation == 2)
        assert(A.cols == B.cols);

    Mat graphcut;
    Mat graphcut_and_cutline;

    int xoffset = 0;
    int yoffset = 0;
    int _rows, _cols; //Of Mat

    if (orientation == 1)
    {
        _rows = A.rows;
        _cols = A.cols + B.cols - overlap;
        xoffset = A.cols - overlap;
    }
    else if ( orientation == 2)
    {
        _rows = A.rows + B.rows - overlap;
        _cols = A.cols;
        yoffset = A.rows - overlap;
    }


    Mat no_graphcut(_rows, _cols, A.type() );
    A.copyTo(no_graphcut(Rect(0, 0, A.cols, A.rows)));
    B.copyTo(no_graphcut(Rect(xoffset, yoffset, B.cols, B.rows)));
    imshow("no graphcut ", no_graphcut);
    

    
    int est_nodes;
    if (orientation == 1)      
        est_nodes = A.rows * overlap;
    else  
        est_nodes = A.cols * overlap;
    int est_edges = est_nodes * 4;

    GraphType g(est_nodes, est_edges);

    for(int i=0; i < est_nodes; i++) {
        g.add_node();
    }

    if (orientation == 1)
    {
        // Set the source/sink weights
        for(int y=0; y < A.rows; y++) {
            g.add_tweights(y*overlap + 0, INT_MAX, 0);
            g.add_tweights(y*overlap + overlap-1, 0, INT_MAX);
        }

        // Set edge weights
        for(int y=0; y < A.rows; y++) { //Change this 
            for(int x=0; x < overlap; x++) {
                int idx = y*overlap + x;

                Vec3b a0 = A.at<Vec3b>(y, xoffset + x);
                Vec3b b0 = B.at<Vec3b>(y, x);
                double cap0 = norm(a0, b0);

                // Add right edge
                if(x+1 < overlap) {
                    Vec3b a1 = A.at<Vec3b>(y, xoffset + x + 1);
                    Vec3b b1 = B.at<Vec3b>(y, x + 1);

                    double cap1 = norm(a1, b1);

                    g.add_edge(idx, idx + 1, (int)(cap0 + cap1), (int)(cap0 + cap1));
                }

                // Add bottom edge
                if(y+1 < A.rows) {
                    Vec3b a2 = A.at<Vec3b>(y+1, xoffset + x);
                    Vec3b b2 = B.at<Vec3b>(y+1, x);

                    double cap2 = norm(a2, b2);

                    g.add_edge(idx, idx + overlap, (int)(cap0 + cap2), (int)(cap0 + cap2));
                }
            }
        }
    }
    else 
    {
        //Set the source/sink weights
        for(int x=0; x < A.cols; x++) {
            g.add_tweights(x*overlap + 0, INT_MAX, 0); // Add the Terminal nodes 
            g.add_tweights(x*overlap + overlap - 1, 0, INT_MAX);
        }
        for(int x=0; x < A.cols; x++) {
            for( int y=0; y < overlap; y++)  {
                int idx = x*overlap + y;

                Vec3b a0 = A.at<Vec3b>(y, xoffset + x);
                Vec3b b0 = B.at<Vec3b>(y, x);
                double cap0 = norm(a0, b0);

                
                 // Add bottom edge
                if(y+1 < overlap) {
                    Vec3b a1 = A.at<Vec3b>(yoffset + y + 1, x);
                    Vec3b b1 = B.at<Vec3b>(y + 1,x);
                    double cap1 = norm(a1, b1);
                    g.add_edge(idx, idx + 1, (int)(cap0 + cap1), (int)(cap0 + cap1));
                }

                // Add right edge
                if(x+1 < A.cols) {
                    Vec3b a2 = A.at<Vec3b>(yoffset + y, x+1);
                    Vec3b b2 = B.at<Vec3b>(y, x+1);
                    double cap2 = norm(a2, b2);
                    g.add_edge(idx, idx + overlap, (int)(cap0 + cap2), (int)(cap0 + cap2));
                }
            }
             
        }
    }
    
    int flow = g.maxflow();
    cout << "max flow: " << flow << endl;

    graphcut = no_graphcut.clone();
    graphcut_and_cutline = no_graphcut.clone();

    int idx = 0;
    if (orientation == 1)
    {
        for(int y=0; y < A.rows; y++) {
            for(int x=0; x < overlap; x++) {
                if(g.what_segment(idx) == GraphType::SOURCE) {
                    graphcut.at<Vec3b>(y, xoffset + x) = A.at<Vec3b>(y, xoffset + x);
                }
                else {
                    graphcut.at<Vec3b>(y, xoffset + x) = B.at<Vec3b>(y, x);
                }
                graphcut_and_cutline.at<Vec3b>(y, xoffset + x) =  graphcut.at<Vec3b>(y, xoffset + x);

                // Draw the cut
                if(x+1 < overlap) {
                    if(g.what_segment(idx) != g.what_segment(idx+1)) {
                        graphcut_and_cutline.at<Vec3b>(y, xoffset + x) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(y, xoffset + x + 1) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(y, xoffset + x - 1) = Vec3b(0,255,0);
                    }
                }
                // Draw the cut
                if(y > 0 && y+1 < A.rows) {
                    if(g.what_segment(idx) != g.what_segment(idx + overlap)) {
                        graphcut_and_cutline.at<Vec3b>(y-1, xoffset + x) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(y, xoffset + x) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(y+1, xoffset + x) = Vec3b(0,255,0);
                    }
                }
                idx++;
            }
        }  
    }
    
    if (orientation == 2)
    {
        for(int x=0; x < A.cols; x++) {
            for( int y=0; y < overlap; y++)  {
                if(g.what_segment(idx) == GraphType::SOURCE) {
                    graphcut.at<Vec3b>(yoffset + y, x) = A.at<Vec3b>(yoffset + y, x);
                }
                else {
                    graphcut.at<Vec3b>(yoffset + y, x) = B.at<Vec3b>(y, x);
                }
                graphcut_and_cutline.at<Vec3b>(yoffset + y,  x) =  graphcut.at<Vec3b>(yoffset + y, x);

                // Draw the cut
                if(y+1 < overlap) {
                    if(g.what_segment(idx) != g.what_segment(idx+1)) {
                        graphcut_and_cutline.at<Vec3b>(yoffset + y, x) = Vec3b(0,0255,0);
                        graphcut_and_cutline.at<Vec3b>(yoffset + y + 1, x) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(yoffset + y - 1, x) = Vec3b(0,255,0);
                    }
                }

                // Draw the cut
                //if(y > 0 && y+1 < A.rows) {
                if(x > 0 && x+1 < A.cols) {
                    if(g.what_segment(idx) != g.what_segment(idx + overlap)) {
                        graphcut_and_cutline.at<Vec3b>(yoffset + y, x-1) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(yoffset + y, x) = Vec3b(0,255,0);
                        graphcut_and_cutline.at<Vec3b>(yoffset + y, x+1) = Vec3b(0,255,0);
                    }
                }
                idx++;
            }
        }
    }

    /*imwrite("graphcut.jpg", graphcut);
    imwrite("graphcut_and_cut_line.jpg", graphcut_and_cutline);*/
    imshow("graphcut and cut line", graphcut_and_cutline);
    imshow("graphcut", graphcut);

    //waitKey();

    return graphcut;
}

Mat FinalImage::selectSubset(Mat &originalImg, int width_patch, int height_patch)
{
    //copy a sub matrix of X to Y with starting coodinate (startX,startY)
    // and dimension (cols,rows)
    int startX = rand() % (originalImg.cols - width_patch);
    int startY = rand() % (originalImg.rows - height_patch);
    Mat tmp = originalImg(cv::Rect(startX, startY, width_patch, height_patch)); 
    Mat subset;
    tmp.copyTo(subset);
    return subset;

}

double FinalImage::msqe(Mat &target, Mat &patch)
{
    int i, j;
    double eqm, tmpEqm = 0;
    int height = target.rows;
    int width = target.cols;
    uint8_t* pixelPtrT = (uint8_t*)target.data;
    uint8_t* pixelPtrP = (uint8_t*)patch.data;
    Scalar_<uint8_t> bgrPixelT;
    int cnT = target.channels();
    int cnP = patch.channels();
    double valTarget, valPatch = 0;
    double R, G, B = 0;

    for ( i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {

            B = pixelPtrT[i*target.cols*cnT + j*cnT + 0]; // B
            B -= pixelPtrP[i*patch.cols*cnP + j*cnP + 0]; // B

            G = pixelPtrT[i*target.cols*cnT + j*cnT + 1]; // G
            G -= pixelPtrP[i*patch.cols*cnP + j*cnP + 1]; // G

            R = pixelPtrT[i*target.cols*cnT + j*cnT + 2]; // R
            R -= pixelPtrP[i*patch.cols*cnP + j*cnP + 2]; // R

            tmpEqm = sqrt((B + G + R) * (B + G + R));
            eqm += tmpEqm;

            valTarget = 0;
            valPatch = 0;
          
        }
    }

    eqm /= height * width;
    return eqm;
}

Patch FinalImage::getRandomPatch(std::vector<Patch> patchesList)
{
    //return random error from list of best errors
    //If we don't do this, the chosen patches will look extremely similar
    std::vector<Patch> bestErrorsList;

    double tempError;
    Patch bestPatch; 
    double minError = 2.0; //Chose a value for acceptable error
    
    //Check that each new error stored in PatchesList is in fact new
    std::vector<Patch>::iterator isRepeatedElem;
    bool repeatedElem;
    while (bestErrorsList.size() < 50)
    {
        for (int i = 0; i < patchesList.size(); i++)
        {
            isRepeatedElem = find_if(bestErrorsList.begin(), bestErrorsList.end(), findRepeatedPatch(patchesList[i].error));
            if (isRepeatedElem != bestErrorsList.end()) repeatedElem = true;
            else repeatedElem = false;

            if (patchesList[i].error < minError && !repeatedElem )
            {
                bestErrorsList.push_back(patchesList[i]);
            }
        }
        minError += 3;
    }


    int i = rand() % (bestErrorsList.size() - 1); //return random error from list of best errors
    if ( bestErrorsList.size() != 0) {    
        bestPatch = bestErrorsList[i];
    }
    if (bestErrorsList.size() == 0)
        cout << "WARNING, empty list" << endl; //This should never happen

    return bestPatch;
}

Mat FinalImage::placeRandomly(Patch patch, Mat &img)
{
    int posXPatch =  0;
    int posYPatch = 0;
    srand(time(NULL)); //Seed to get randmon patches


    for (int patchesInY = 0; patchesInY < newimg.rows/patch.height; patchesInY++)
   {
        for (int patchesInX = 0; patchesInX < newimg.cols/patch.width; patchesInX++)
        {
            patch.image = selectSubset(img, patch.width, patch.height); //subselection from original texture
            Rect rect2(posXPatch, posYPatch, patch.width, patch.height);
            patch.image.copyTo(newimg(rect2));
            posXPatch += patch.width;
        }
        posXPatch = 0;
        posYPatch += patch.height;
    }
   
   return newimg;

}

Mat FinalImage::choseTypeTexture(Mat &img, Mat &img2, Mat &img3, Patch &p, Grid &g, int x, int y) //Chose either background (0) or details (1) texutre
{
    if (g.grid[x][y] == 0){
        p.typeOfTexture = 0;
        return img;
    }
    else if (g.grid[x][y] == 1){
        p.typeOfTexture = 1;
        return img2;
    }
    else if (g.grid[x][y] == 2){
        p.typeOfTexture = 2;
        return img3;
    }   
}


Mat FinalImage::addBlending(Mat &_patch, Mat &_template, Point center)
{
    Mat src = _patch;
    Mat dst = _template(Rect(0, 0, _template.cols, _template.rows));
    // Create an all white mask
    Mat src_mask = 255 * Mat::ones(src.rows, src.cols, src.depth());
    Mat normal_clone;
    seamlessClone(src, dst, src_mask, center, normal_clone, NORMAL_CLONE); 
    //circle( normal_clone, center, 5.0, Scalar( 0, 50, 255 ), 1, 8 );
    return normal_clone;
}

Mat FinalImage::textureSynthesis(Patch patch, Patch target, Mat &img, Mat &img2, Mat &img3, int backgroundPorcentage, int detailsPorcentage)
{
    Patch newTarget(img); //Target
    Patch bestP(img);     //Patch

    Point center;

    //Mats for cloning
    Mat src;
    Mat dst;
    Mat normal_clone;

    Mat selectedTexture, _newImg, _finalImage;

    overlap = patch.width / 6; 
    offset = patch.width - overlap; 
    posYPatch = posYTarget = 0;
    posXPatch = patch.width - overlap;
    gridSize = (newimg.cols/patch.width) * (newimg.rows/patch.height);

    //Size of grid
    gridX = (width / patch.width) + 1; //plus one because with the overlaping of patches, there is space for one more
    gridY = (height / patch.height) ;

    //Create grid with random distribution for either background or texture
    Grid grid(gridX, gridY); 
    grid.fill(backgroundPorcentage); 

    //Create first target
    selectedTexture = choseTypeTexture(img, img2, img3, patch, grid, 0,0); 
    target.image = selectSubset(selectedTexture, target.width, target.height); //Create a smaller subset of the original image 
    Rect rect(0,0, target.width, target.height);
    target.image.copyTo(newimg(rect));
    
    cout << "size: " << grid.grid[1].size() << endl;
    for (int patchesInY = 0; patchesInY < grid.grid[1].size(); patchesInY++)
   {
        for (int patchesInX = 1; patchesInX < grid.grid.size(); patchesInX++) 
        {    
            //Choose texture background or foreground 
            selectedTexture = choseTypeTexture(img, img2, img3, patch, grid, patchesInX, patchesInY);
            
            //Start comparing patches (until error is lower than tolerance)
            for (int i = 0; i < 500 ; i++) //This alue needs to be at least 50
            {
                //Set image to patch
                patch.image = selectSubset(selectedTexture, patch.width, patch.height); //subselection from original texture
      
                //Create ROIs
                patch.roiOfPatch = patch.image(Rect(0, 0, overlap, patch.height));
                patch.roiOfTarget = target.image(Rect(offset, 0, overlap, target.height));
                patch.halfOfTarget = target.image(Rect(target.width/4, 0, target.width-(target.width/4), target.height));

                err = msqe(patch.roiOfTarget, patch.roiOfPatch); //Get MSQE

                if (patchesInY > 0) //if is the second or bigger row
                {
                    patch.roiOfTopPatch = patch.image(Rect(0, 0, patch.width, overlap));
                    patch.roiOfBotTarget = newimg(Rect(posXPatch, posYPatch - overlap, patch.width, overlap));
                 
                    err += msqe(patch.roiOfTopPatch, patch.roiOfBotTarget);
                    err = err/2; 
                }

                patch.error = err;
                _patchesList.push_back(patch);
                err = 0;
            }

            //chose random patch from best errors list
            bestP = getRandomPatch(_patchesList);

            _newImg = newimg(Rect(0, posYPatch, posXPatch + overlap, bestP.image.rows)); //temporal target
            _newImg = graph_Cut( _newImg, bestP.image, overlap, 1);
            _newImg.copyTo(newimg(Rect(0, posYPatch, _newImg.cols, _newImg.rows)));

            //Set new target, which is the best patch of this iteration       
            target.image = bestP.image;
            posXPatch += patch.width - overlap;
            _patchesList.clear();
        }
        
        posXPatch = patch.width - overlap; //Update posision of X
        posYPatch += patch.height;//New patch of the next row (which is the new first target)
        newTarget.roiOfBotTarget = newimg(Rect(0, posYPatch - overlap , patch.width, overlap));

        if (patchesInY + 1 != grid.grid[1].size())
        {
            for (int i = 0; i < 100; i++)
            {
                //selectedTexture = choseTypeTexture(img, img2, patch, grid, 0, patchesInY+1);
                newTarget.image = selectSubset(img, newTarget.width, newTarget.height); //subselection from original texture            
                //Create ROIs
                newTarget.roiOfTopPatch = newTarget.image(Rect(0, 0, newTarget.width, overlap));   
                //Calculate errors
                err = msqe(newTarget.roiOfTopPatch, newTarget.roiOfBotTarget);
                newTarget.error = err;
                _patchesList.push_back(newTarget);
            }
            cout << "y: " << patchesInY << endl;
            //chose best error
            bestP = getRandomPatch(_patchesList);
            newTarget.image = bestP.image;
            target = newTarget;
            target.image.copyTo(newimg(Rect(0, posYPatch, target.image.cols, target.image.rows)));
            _patchesList.clear();     
        }
        
    }

    posYPatch = 0;
    posXPatch = patch.width - overlap;
    int widht_Final_image = newimg.cols - overlap * 2;
    Mat _patch, _template, gc, synthesised_Image;
    int newTmpY = 0; //new position in y to do the GC

    synthesised_Image = Mat::zeros(newimg.rows, widht_Final_image, CV_64FC3);
    synthesised_Image.convertTo(synthesised_Image, CV_8UC1);
    
    _template = newimg(Rect(0,posYPatch, widht_Final_image, patch.height));
    _patch = newimg(Rect(0,posYPatch + patch.height, widht_Final_image, patch.height)); 
    
    for (int patchesInY = 0; patchesInY <grid.grid[1].size()-1 ; patchesInY++)
    {
        if (patchesInY != 0)
        {
            _template = synthesised_Image(Rect(0,posYPatch - (overlap * patchesInY), widht_Final_image, patch.height));
            _patch = newimg(Rect(0,posYPatch + patch.height, widht_Final_image, patch.height)); 
        }

        //Apply GC
        gc = graph_Cut(_template, _patch, overlap, 2);
        imshow("gc", gc);
        gc.copyTo(synthesised_Image(Rect(0,newTmpY, gc.cols, gc.rows)));

        //Apply blending
        // The location of the center of the src in the dst
        Point center(_patch.cols/2 , _patch.rows + overlap);
        normal_clone = addBlending(_patch, gc, center);
        normal_clone.copyTo(gc(Rect(0, 0, normal_clone.cols, normal_clone.rows)));

        //Add the cut + blending to final image
        gc.copyTo(synthesised_Image(Rect(0,newTmpY, gc.cols, gc.rows)));
        _finalImage = synthesised_Image(Rect(0,0, synthesised_Image.cols, (grid.grid[1].size() * patch.height) - (overlap * grid.grid[1].size()-2) ));

        posYPatch += patch.height;
        newTmpY += patch.height - overlap;
        //imshow("_template", _template);
        //imshow("_patch", _patch);
        imwrite("patch.jpg", _patch);
        imwrite("template.jpg", _template);
        
    }
    
    imwrite("final.jpg", _finalImage);
    return _finalImage;
}