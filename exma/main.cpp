#include <iostream> // for printing and reading user input if needed
#include <experimental/filesystem> //For file manipulation
#include <string>
#include "CImg.h"
#include "cxxopts.hpp"

namespace fs = std::experimental::filesystem;

struct EnvironmentVariables //This struct will hold some important environment values between contexts
{
	//These are all dictated by the files in the specified folder
	int HEIGHT, WIDTH, DEPTH, THRESHOLD, LAYER_BLUR;
	bool DISPLAY, SAVE;
	bool A_BIOFILM, A_CONCENTRATION;
	std::string imageFolderName;
	std::vector<std::string> imageFileNames;

	//These are all dictated by a config file
	float pixel_dimension_um;
};

void IMGreader(const std::string imageName, int current_depth, int*** DATA, EnvironmentVariables* env);
void setEnvironmentVariables(EnvironmentVariables* env);
void loadImages(cimg_library::CImgList<unsigned char>* list,EnvironmentVariables* env);

int main(int argc, char* argv[])
{
	//Create the environment variables for this analysis
	EnvironmentVariables env;

	//Parse Command line Args
	cxxopts::Options options("exma", "Analysis of extracelluar matrix and biofilm data");

	try {
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options.add_options("Input")
			("f,folder", "Folder name containing Image data", cxxopts::value<std::string>(env.imageFolderName))
			("t,threshold", "Intensity threshold for detection", cxxopts::value<int>(env.THRESHOLD)->default_value("50"))
			("layer_blur", "2D image blur radius", cxxopts::value<int>(env.LAYER_BLUR)->default_value("0"))
			;

		options.add_options("Analysis")
			("b,biofilm", "Biofilm Thickness", cxxopts::value<bool>(env.A_BIOFILM))
			("c,concentration", "Concentration Gradient", cxxopts::value<bool>(env.A_CONCENTRATION))
			;

		options.add_options("Output")
			("s,save", "Save all outputs as images", cxxopts::value<bool>(env.SAVE))
			("d,display", "Display all outputs to screen", cxxopts::value<bool>(env.DISPLAY))
			;

		options.add_options()
			("h, help", "Print help")
			;

		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			std::cout << options.help({}) << std::endl;
			return 0;
		}
	}
	catch (const cxxopts::OptionException& e) {
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(-1);
	}

	//Verify that environment variables are correct, then set file names for analysis
	try {
		setEnvironmentVariables(&env);
	}
	catch (const char* msg) {
		std::cerr << msg << std::endl;
		return 0;
	}

	std::string imageName;
	cimg_library::CImgList<unsigned char> imageList;
	int totalImages = 0, currentImage = 0;

	//Verify that environment variables are correct, then set file names for analysis
	try {
		loadImages(&imageList, &env);
	}
	catch (const char* msg) {
		std::cerr << msg << std::endl;
		return 0;
	}

	cimg_library::CImg<unsigned char> output = imageList[1];
	int** OUTPUT;

	OUTPUT = new int*[env.HEIGHT];
	for (int i = 0; i < env.HEIGHT; i++)
	{
		OUTPUT[i] = new int[env.WIDTH];
	}

	for (int i = 0; i < env.HEIGHT; i++)
	{
		for (int j = 0; j < env.WIDTH; j++)
		{
			OUTPUT[i][j] = 0;
		}
	}

	int maxVal = 0;
	int threshold = env.THRESHOLD;

	for (int i = 0; i < env.HEIGHT; i++)
	{
		for (int j = 0; j < env.WIDTH; j++)
		{
			int bottom = -1, top = -1;
				for (int im = 0; im < env.DEPTH; im++)
				{
					if (imageList[im](j, i, 0, 1) > threshold)
					{
						if (bottom == -1)
						{
							bottom = im;
						}
						top = im;
					}
				}
				if (bottom != -1)
				{
					for (int im = 0; im <= top - bottom; im++)
					{
						OUTPUT[i][j] += 255/env.DEPTH;
					}
				}
		}
	}

	for (int i = 0; i < env.HEIGHT; i++)
	{
		for (int j = 0; j < env.WIDTH; j++)
		{
			for (int RGB = 0; RGB < 3; RGB++)
			{
				for (int im = 0; im < env.DEPTH; im++)
				{
					output(j, i, RGB) = OUTPUT[i][j];
				}
			}
		}
	}

	if (env.DISPLAY)
	{
		cimg_library::CImgDisplay main_disp(output, "Biofilm Thickness");

		main_disp.resize(1000, 1000, true);

		while (!main_disp.is_closed()) {
			main_disp.wait();
		}
	}

	try {
		//IMGreader(blah blah blah);
	} catch (const char* msg) {
		std::cerr << msg << std::endl;
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

		std::cout << (*it) << std::endl;
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

void IMGreader(const std::string imageName, int current_depth, int*** DATA, EnvironmentVariables* env)
{
	const char *charImageName = imageName.c_str();
	cimg_library::CImg<unsigned char> src(charImageName);

	if (src.width() < 1 || src.height() < 1)
	{
		throw "Image with 0 height or width : " + imageName;
	}

	if (src.width() != env->WIDTH || src.height() != env->HEIGHT)
	{
		throw "Inconsistent image heights or widths : " + imageName;
	}
	else
	{
		for (int RGB = 0; RGB < 3; RGB++)
		{
			for (int i = 0; i < env->HEIGHT; i++)
			{
				for (int j = 0; j < env->WIDTH; j++)
				{
					DATA[i][j][RGB] = (int)src(j, i, 0, RGB);
				}
			}
		}
	}
}