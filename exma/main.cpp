#include <iostream> // for printing and reading user input if needed
#include <experimental/filesystem> //For file manipulation
#include <string>
#include "CImg.h" //Image processor
#include "cxxopts.hpp" //Command line argument parser

#define _USE_MATH_DEFINES
#include <math.h>

namespace fs = std::experimental::filesystem;

//This struct will hold some important environment values between contexts
struct EnvironmentVariables
{
	//These are all dictated by the files in the specified folder
	int HEIGHT, WIDTH, DEPTH, THRESHOLD, LAYER_BLUR, MAX_SPACE;
	bool DISPLAY, SAVE, VERBOSE;
	bool A_BIOFILM, A_CONCENTRATION;
	std::string imageFolderName;
	std::vector<std::string> imageFileNames;

	//Config variables
	float CROSS_AREA = 350.f*75.f; // micrometers squared
	float FLOW_RATE = 4.f; //microliters per hour
	float PIXEL_WIDTH = 0.1565983f; //micrometers
	float LAYER_THICKNESS = 1.f; //micrometers
	float DIFFUSIVITY = 100.f; //micrometers squared per second

	//These are all dictated by a config file
	float pixel_dimension_um;
};

//Simple 2d vector
struct vec2
{
	int x, y;
};

//Functions
void setEnvironmentVariables(EnvironmentVariables* env);
void loadImages(cimg_library::CImgList<unsigned char>* list,EnvironmentVariables* env);
void parseArgs(int argc, char* argv[], EnvironmentVariables* env);
vec2 findTip(cimg_library::CImg<unsigned char>* image);
cimg_library::CImg<unsigned char> calcBiofilm(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env);
cimg_library::CImg<unsigned char> calcConcentrationGradient(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env, vec2 startingPoint);

int main(int argc, char* argv[])
{
	/////////////////////////////////////////////////////////////////////////////////
	//SET UP ENVIRONMENT

		//Create the environment variables for this analysis
		EnvironmentVariables env;

		//Parse command line args
		parseArgs(argc, argv, &env);
	
		//Verify that environment variables are correct, then set file names for analysis	
		try {
			setEnvironmentVariables(&env);
		}
		catch (const char* msg) {
			std::cerr << msg << std::endl;
			return 0;
		}

		//Make the list of images representing each vertical slice of the 3D data
		cimg_library::CImgList<unsigned char> imageList;
	
		//Verify that environment variables are correct and load each image into the list
		try {
			loadImages(&imageList, &env);
		}
		catch (const char* msg) {
			std::cerr << msg << std::endl;
			return 0;
		}
	/////////////////////////////////////////////////////////////////////////////////



	/////////////////////////////////////////////////////////////////////////////////
	//DO ANALYSIS AND COMPUTATIONS
		
		cimg_library::CImg<unsigned char> biofilm_image, concentration_image;

		//If concentration gradient needs to be calculated
		if (env.A_CONCENTRATION)
		{
			//First, find x, y of bottom-most image that represents the tip of mixing point
			findTip(&imageList[0]);

			vec2 startingLoc;
			startingLoc.x = 250;
			startingLoc.y = env.HEIGHT / 2 + 40;

			concentration_image = calcConcentrationGradient(&imageList, &env, startingLoc);
		}

		if (env.A_BIOFILM)
		{
			biofilm_image = calcBiofilm(&imageList, &env);
		}

	

	if (env.DISPLAY)
	{
		if (env.A_BIOFILM)
		{
			cimg_library::CImgDisplay main_disp(biofilm_image, "Biofilm Thickness");

			main_disp.resize(1000, 1000, true);

			while (!main_disp.is_closed()) {
				main_disp.wait();
			}
		}
		if (env.A_CONCENTRATION)
		{
			cimg_library::CImgDisplay main_disp(concentration_image, "Concentration Gradient");

			main_disp.resize(1000, 1000, true);

			while (!main_disp.is_closed()) {
				main_disp.wait();
			}
		}
		cimg_library::CImg<unsigned char> combined = biofilm_image;
		for (int i = 0; i < env.HEIGHT; i++)
		{
			for (int j = 0; j < env.WIDTH; j++)
			{
				for (int RGB = 0; RGB < 3; RGB++)
				{
					for (int im = 0; im < env.DEPTH; im++)
					{
						if (RGB == 0)
						{
							combined(j, i, RGB) = biofilm_image(j, i, RGB);
						}
						if (RGB == 2)
						{
							combined(j, i, RGB) = concentration_image(j, i, RGB);
						}
					}
				}
			}
		}

		if (true)
		{
			cimg_library::CImgDisplay main_disp(combined, "Concentration Gradient");

			main_disp.resize(1000, 1000, true);

			while (!main_disp.is_closed()) {
				main_disp.wait();
			}
		}
	}

	try {
		//IMGreader(blah blah blah);
	} catch (const char* msg) {
		std::cerr << msg << std::endl;
	}
}

void parseArgs(int argc, char* argv[], EnvironmentVariables* env)
{
	//Parse Command line Args
	cxxopts::Options options("exma", "Analysis of extracelluar matrix and biofilm data");

	try {
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options.add_options("Input")
			("f,folder", "Folder name containing image data", cxxopts::value<std::string>(env->imageFolderName))
			("t,threshold", "Intensity threshold for detection", cxxopts::value<int>(env->THRESHOLD)->default_value("50"))
			("layer_blur", "2D image blur radius", cxxopts::value<int>(env->LAYER_BLUR)->default_value("0"))
			("max_space", "Largest allowable vertical gap", cxxopts::value<int>(env->MAX_SPACE)->default_value("100"))
			;

		options.add_options("Analysis")
			("b,biofilm", "Biofilm thickness", cxxopts::value<bool>(env->A_BIOFILM))
			("c,concentration", "Concentration gradient", cxxopts::value<bool>(env->A_CONCENTRATION))
			;

		options.add_options("Output")
			("s,save", "Save all outputs as images", cxxopts::value<bool>(env->SAVE))
			("d,display", "Display all outputs to screen", cxxopts::value<bool>(env->DISPLAY))
			;

		options.add_options()
			("h, help", "Print help")
			("v,verbose", "Verbose mode", cxxopts::value<bool>(env->VERBOSE))
			;

		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			std::cout << options.help({}) << std::endl;
			exit(0);
		}
	}
	catch (const cxxopts::OptionException& e) {
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(-1);
	}
}

void setEnvironmentVariables(EnvironmentVariables* env)
{
	if (!fs::exists(env->imageFolderName))
	{
		throw ("Image folder path " + env->imageFolderName + " does not exist").c_str();
	}

	int number_of_images = 0, number_of_non_images = 0;

	env->imageFileNames = {};

	for (auto & p : fs::directory_iterator(env->imageFolderName))
	{
		//Convert Path to string
		static std::string filename;
		filename = p.path().u8string();

		//Check if string is supported image type
		if (filename.find(".") == -1)
		{
			throw "Image folder must not contain subfolders";
		}

		//Set list of supported file types
		std::vector<std::string> supported_types;
		supported_types = { "bmp" , "BMP" };

		if (std::find(supported_types.begin(), supported_types.end(), filename.substr(filename.find_last_of(".") + 1)) != supported_types.end()) {
			env->imageFileNames.emplace_back(filename);

			number_of_images++;
		}
		else {
			number_of_non_images++;
		}
	}

	env->DEPTH = number_of_images;

	if (number_of_images == 0)
		throw "No supported images in directory";

	if (number_of_images == 1)
		throw "Multiple images required for analysis";

	if (number_of_non_images == 1)
		std::cout << "Found " << number_of_non_images << " unsupported file. This will be ignored" << std::endl;
	else if (number_of_non_images > 1)
		std::cout << "Found " << number_of_non_images << " unsupported files. These will be ignored" << std::endl;
}

void loadImages(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env)
{
	bool env_init = false;
	for (std::vector<std::string>::iterator it = env->imageFileNames.begin(); it != env->imageFileNames.end(); ++it) {

		if (env->VERBOSE)
		{
			std::cout << "Loading image: " << (*it) << std::endl;
		}
		const char *charImageName = (*it).c_str();
		cimg_library::CImg<unsigned char> src(charImageName);

		//If it is the first image in the list, use this to determine environment variables
		if (!env_init)
		{
			env->HEIGHT = src.height();
			env->WIDTH = src.width();
			env_init = true;
		}
		//If not, make sure the dimensions are consistent
		else if(src.width() != env->WIDTH || src.height() != env->HEIGHT)
		{
			throw "Inconsistent image dimensions: please check that all images in folder are the same dimensions";
		}
		src.blur(env->LAYER_BLUR);
		list->insert(src);
	}
}

vec2 findTip(cimg_library::CImg<unsigned char>* image)
{
	//This function will iterate through an image and determine where the most likely location of the 'mixing point' or 'tip' is
	return vec2();
}

cimg_library::CImg<unsigned char> calcBiofilm(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env)
{
	cimg_library::CImg<unsigned char> output = (*list)[1];
	int** OUTPUT;

	OUTPUT = new int*[env->HEIGHT];
	for (int i = 0; i < env->HEIGHT; i++)
	{
		OUTPUT[i] = new int[env->WIDTH];
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			OUTPUT[i][j] = 0;
		}
	}

	int maxVal = 0;

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			int counter = 0, confirmed = 0, last = -1;
			for (int im = 0; im < env->DEPTH; im++)
			{
				if ((*list)[im](j, i, 0, 1) > env->THRESHOLD)
				{
					if (counter > env->MAX_SPACE)
					{
						counter = 0;
					}
					confirmed = confirmed + counter + 1;
					last = im;
					counter = 0;
				}
				else if (last != -1)
				{
					counter++;
				}
			}
			if (confirmed > 0)
			{
				for (int im = 0; im < confirmed; im++)
				{
					OUTPUT[i][j] += 255 / env->DEPTH;
				}
			}
		}
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			for (int RGB = 0; RGB < 3; RGB++)
			{
				for (int im = 0; im < env->DEPTH; im++)
				{
					output(j, i, RGB) = OUTPUT[i][j];
				}
			}
		}
	}

	return output;
}

cimg_library::CImg<unsigned char> calcConcentrationGradient(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env, vec2 startingPoint)
{
	cimg_library::CImg<unsigned char> output = (*list)[1];
	int** OUTPUT;

	float C_max = 255.f*255.f*255.f, D = 100.f, h = float(env->HEIGHT) * env->PIXEL_WIDTH / 2.f, L = float(env->HEIGHT)* env->PIXEL_WIDTH ,flow_rate = env->FLOW_RATE*1000000000.f/(60.f*60.f*env->CROSS_AREA);
	int C = 0;

	OUTPUT = new int*[env->HEIGHT];
	for (int i = 0; i < env->HEIGHT; i++)
	{
		OUTPUT[i] = new int[env->WIDTH];
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			if (j < startingPoint.x)
			{
				if (i > startingPoint.y)
				{
					C = 0;
				}
				else
				{
					C = C_max;
				}
			}
			else
			{
				float sum_part = 0.f;
				int k_max = 500;
				for (int k = 1; k < k_max; k++)
				{
					sum_part += (1 / float(k))*
						std::sinf(float(k)*M_PI*h / L)*
						std::exp(-D * float(k)*float(k)*M_PI*M_PI*float(j - startingPoint.x + 1)* env->PIXEL_WIDTH / (L* L*flow_rate))*
						std::cosf(float(k)*M_PI*(float(i - startingPoint.y)* env->PIXEL_WIDTH + h) / L);
				}
				C = C_max*(sum_part * 2.f / M_PI + h / L);
				if (C < 0)
				{
					C = 0;
				}
				if (C > C_max)
				{
					C = C_max;
				}
			}
			OUTPUT[i][j] = C;
		}
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			output(j, i, 0) = OUTPUT[i][j]/(255*255);
			output(j, i, 1) = (OUTPUT[i][j]%(256*256))/255;
			output(j, i, 2) = OUTPUT[i][j]%256;
		}
	}

	return output;
}