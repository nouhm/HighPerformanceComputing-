#include <iostream>
#include <math.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include<stdio.h>
#include<stdlib.h>
#include<omp.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//********************Read Image and save it to local arrayss********	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("C:\\Users\\user\\source\\repos\\HPC_ProjectTemplate_OpenMP\\HPC_ProjectTemplate" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

int** padImgMatrix(int rows, int cols, int** imgMatrix)
{
	int** paddedImgMatrix = new int* [rows + 2];

	for (int i = 0; i < rows + 2; ++i) {
		paddedImgMatrix[i] = new int[cols + 2];
	}

	for (int i = 0; i < rows + 2; i++) {
		for (int j = 0; j < cols + 2; j++) {
			paddedImgMatrix[i][j] = 0;
		}
	}
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {

			paddedImgMatrix[i + 1][j + 1] = imgMatrix[i][j];
				// *((imgMatrix + i * cols) + j);
		}
	}
	return paddedImgMatrix;
}

int** vectorToMatrix(int* paddedImgVector, int rows, int cols) {
	int** paddedImgMatrix = new int* [rows];
	for (int i = 0; i < rows; i++) {
		paddedImgMatrix[i] = new int[cols];
		for (int j = 0; j < cols; j++) {
			paddedImgMatrix[i][j] = paddedImgVector[i * cols + j];
		}
	}
	return paddedImgMatrix;
}

int* matrixToVector(int rows, int cols, int** paddedImgMatrix) {
	int* paddedImgVector = new int[rows * cols];
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			paddedImgVector[i * cols + j] = paddedImgMatrix[i][j];
		}
	}
	return paddedImgVector;
}

int main(int argc, char** argv)
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0, kernel = 3;


	System::String^ imagePath;
	std::string img;
	img = "C:\\Users\\user\\source\\repos\\HPC_ProjectTemplate_OpenMP\\HPC_ProjectTemplate\\lena.png";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	start_s = clock();


	int** imgMatrix = vectorToMatrix(imageData, ImageHeight, ImageWidth); 

	
	int** paddedImgMatrix = padImgMatrix(ImageHeight, ImageWidth, imgMatrix);

	int paddedImg = ImageHeight + 2;
	int sum = 0;
	int kernelMatrix[3][3] = {
		{0,-1,0},
		{-1,4,-1},
		{0,-1,0}
	};
	int imgBorder = kernel / 2;
	int borderlessImg = paddedImg - imgBorder;
	int** newImgMatrix = (int*)malloc(ImageHeight * sizeof(int));
	int nthreads = atoi(argv[1]); 

	for (int i = 0;i < ImageHeight;i++)
	{
		newImgMatrix[i] = (int*)malloc(ImageHeight * sizeof(int));
	}

#pragma omp for schedule(static) num_threads(nthreads)
	for (int i = imgBorder;i < borderlessImg;i++)
	{
		for (int j = imgBorder;j < borderlessImg;j++)
		{
			sum = 0;
			for (int k = 0;k < kernel;k++)
			{
				for (int l = 0;l < kernel;l++)
				{
					sum += kernelMatrix[k][l] * paddedImgMatrix[i - imgBorder + k][j - imgBorder + l];
				}
			}
			newImgMatrix[i - imgBorder][j - imgBorder] = sum;
		}
	}

	imageData = matrixToVector(ImageHeight, ImageWidth, newImgMatrix); 
	
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	createImage(imageData, ImageWidth, ImageHeight, 1);
	cout << "time: " << TotalTime << endl;

	free(imageData);
	return 0;

}