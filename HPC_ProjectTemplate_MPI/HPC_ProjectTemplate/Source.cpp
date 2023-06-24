#include <iostream>
#include <math.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>
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

	//*******Read Image and save it to local arrayss***	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

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
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("C:\\Users\\user\\source\\repos\\HPC_ProjectTemplate_MPI\\HPC_ProjectTemplate" + index + ".png");
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

int main()
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0, kernel = 3;


	System::String^ imagePath;
	std::string img;
	img = "C:\\Users\\user\\source\\repos\\HPC_ProjectTemplate_MPI\\HPC_ProjectTemplate\\lena.png";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	start_s = clock();

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);
	// Get the number of processes
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	// Get the rank of the process
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Status status;

	if (size > ImageHeight) {
		if (rank == 0)
			cout << "Please run this program with a max of " << ImageHeight << " MPI Processes." << endl;
		MPI_Finalize();
		exit(1);
	}

	int sum = 0;
	int startRow = 0;
	int imgBorder = kernel / 2;
	int borderlessImg = ImageHeight + 2 - imgBorder;
	int rowsPerProcessor = ImageHeight / size;
	cout << "Rows per Processor: " << rowsPerProcessor << endl;
	int sizeOfLocalResult;
	int resultIndex = 0;
	int rowsLeft = ImageHeight - rowsPerProcessor * (size - 1);
	int kernelMatrix[3][3] = {{0, -1, 0},{-1, 4, -1},{0, -1, 0}};

	int** imgMatrix = vectorToMatrix(imageData, ImageHeight, ImageWidth);
	int** paddedImgMatrix = padImgMatrix(ImageHeight, ImageWidth, imgMatrix);

	int* localResult;
	int* finalResult = new int[ImageHeight * ImageWidth];
	int* rcvdResult;
	int** result = new int* [ImageHeight];

	for (int i = 0; i < ImageHeight; i++) {
		result[i] = new int[ImageWidth];
	}


	if (rank == 0) {
		// Send the rows of the paddedImgMatrix to each process
		for (int i = 1; i < size; i++) {
			startRow = i * rowsPerProcessor;
			MPI_Send(&startRow, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		startRow = 0;
	}
	else {
		// Receive the rows of the paddedImgMatrix from the root process
		MPI_Recv(&startRow, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	sizeOfLocalResult = 0;
	localResult = new int[ImageHeight * ImageWidth];

	if (rank == size - 1) {
		for (int i = startRow; i < startRow + rowsLeft; i++)
		{
			for (int j = imgBorder; j < borderlessImg; j++)
			{
				sum = 0;
				for (int k = 0;k < kernel;k++)
				{
					for (int l = 0; l < kernel; l++)
					{
						sum += kernelMatrix[k][l] * paddedImgMatrix[i + k][j - imgBorder + l];
					}
				}
				localResult[sizeOfLocalResult] = sum;
				sizeOfLocalResult++;
			}
		}

	}
	else {
		for (int i = startRow; i < startRow + rowsPerProcessor; i++)
		{
			for (int j = imgBorder; j < borderlessImg; j++)
			{
				sum = 0;
				for (int k = 0;k < kernel;k++)
				{
					for (int l = 0; l < kernel; l++)
					{
						sum += kernelMatrix[k][l] * paddedImgMatrix[i + k][j - imgBorder + l];
					}
				}
				localResult[sizeOfLocalResult] = sum;
				sizeOfLocalResult++;
			}
		}

	}


	cout << endl;
	if (rank != 0) {
		MPI_Send(&sizeOfLocalResult, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(localResult, sizeOfLocalResult, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
	else {
		for (int j = 0; j < sizeOfLocalResult; j++) {
			finalResult[resultIndex] = localResult[j];
			resultIndex++;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		for (int i = 1; i < size; i++) {
			sizeOfLocalResult = 0;
			MPI_Recv(&sizeOfLocalResult, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
			rcvdResult = new int[sizeOfLocalResult];
			MPI_Recv(rcvdResult, sizeOfLocalResult, MPI_INT, i, 0, MPI_COMM_WORLD, &status);

			for (int j = 0; j < sizeOfLocalResult; j++) {
				finalResult[resultIndex] = rcvdResult[j];
				resultIndex++;
			}
		}
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		createImage(finalResult, ImageWidth, ImageHeight, 1);
		cout << "time: " << TotalTime << endl;
		free(imageData);
		return 0;

	}
	MPI_Finalize();

	

}