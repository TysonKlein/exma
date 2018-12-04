#include <iostream> // for printing and reading user input if needed
#include <experimental/filesystem> //For file manipulation
#include <string>
#include <fstream>
#include "CImg.h" //Image processor
#include "cxxopts.hpp" //Command line argument parser

#define _USE_MATH_DEFINES
#include <math.h>

namespace fs = std::experimental::filesystem;

//This struct will hold some important environment values between contexts
struct EnvironmentVariables
{
	//These are all dictated by the files in the specified folder
	int HEIGHT, WIDTH, DEPTH, THRESHOLD, LAYER_BLUR, MAX_SPACE, CONC_STEP, MIX_X, MIX_Y, DISP_PERCENT, DISP_HEIGHT, DISP_WIDTH, CONC_OFFSET, MAX_THICKNESS;
	bool DISPLAY, SAVE, VERBOSE, FACING, OVERLAY, TABLE;
	bool A_BIOFILM, A_CONCENTRATION;
	std::string imageFolderName;
	std::vector<std::string> imageFileNames;

	//Config variables
	float CROSS_AREA = 350.f*75.f; // micrometers squared
	float FLOW_RATE = 4.f; //microliters per hour
	float PIXEL_WIDTH = 0.1565983f; //micrometers
	float LAYER_THICKNESS = 1.f; //micrometers
	float DIFFUSIVITY = 100.f; //micrometers squared per second
};

//Functions
void setEnvironmentVariables(EnvironmentVariables* env);
void loadImages(cimg_library::CImgList<unsigned char>* list,EnvironmentVariables* env);
void parseArgs(int argc, char* argv[], EnvironmentVariables* env);
void drawConcentrationLines(cimg_library::CImg<unsigned char> *concImage, cimg_library::CImg<unsigned char> *drawImage, EnvironmentVariables* env);
cimg_library::CImg<unsigned char> calcBiofilm(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env);
cimg_library::CImg<unsigned char> calcConcentrationGradient(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env);
void drawAt(cimg_library::CImg<unsigned char> *image, int x, int y, int R, int G, int B, int size, EnvironmentVariables* env);
float getConcValue(cimg_library::CImg<unsigned char> *image, int x, int y);
void calcBinnedThickness(cimg_library::CImg<unsigned char> *concImage, cimg_library::CImg<unsigned char> *biofilmImage, EnvironmentVariables* env);

int main(int argc, char* argv[])
{
	/////////////////////////////////////////////////////////////////////////////////
	//SET UP ENVIRONMENT

		//Create the environment variables for this analysis
		EnvironmentVariables env;

		//Parse command line args, setting most environment variables
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
	//If concentration gradient is needed, prompt user to click mixing point
		if (env.A_CONCENTRATION)
		{
			std::cout << "Please click on the mixing point (tip of the divider where two streams meet)" << std::endl;

			cimg_library::CImgDisplay point_disp(imageList[0], "Please click on the mixing point");

			point_disp.resize(env.DISP_WIDTH, env.DISP_HEIGHT, true);

			static bool point_selected = false;

			while (!point_disp.is_closed() && !point_selected) {
				if (point_disp.button() & 1) { // Left button clicked
					if (point_disp.mouse_x() >= 0 && point_disp.mouse_y() >= 0)
					{
						env.MIX_X = point_disp.mouse_x() / (float(env.DISP_PERCENT)*0.01f);
						env.MIX_Y = point_disp.mouse_y() / (float(env.DISP_PERCENT)*0.01f);

						if (env.MIX_X > env.WIDTH / 2)
						{
							env.FACING = true; //Inlet at right
						}
						else
						{
							env.FACING = false; //Inlet at left
						}
						point_selected = true;
					}
				}
				point_disp.wait();
			}
		}
	/////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////
	//DO ANALYSIS AND COMPUTATIONS
		
		//Images to display for both
		cimg_library::CImg<unsigned char> biofilm_image, concentration_image;

		//If concentration gradient needs to be calculated
		if (env.A_CONCENTRATION)
		{
			concentration_image = calcConcentrationGradient(&imageList, &env);
		}

		//If biofilm thickness needs to be calculated
		if (env.A_BIOFILM)
		{
			biofilm_image = calcBiofilm(&imageList, &env);
		}

		if (env.A_CONCENTRATION && env.TABLE && env.A_BIOFILM)
		{
			calcBinnedThickness(&concentration_image, &biofilm_image, &env);
		}

		if (env.A_CONCENTRATION && env.OVERLAY && env.A_BIOFILM)
		{
			drawConcentrationLines(&concentration_image, &biofilm_image, &env);
		}

	if (env.DISPLAY)
	{
		if (env.A_BIOFILM)
		{
			cimg_library::CImgDisplay main_disp(biofilm_image, "Biofilm Thickness");

			main_disp.resize(env.DISP_WIDTH, env.DISP_HEIGHT, true);

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

	biofilm_image.save_bmp("Biofilm_thickness.bmp");
}

void parseArgs(int argc, char* argv[], EnvironmentVariables* env)
{
	//Parse Command line Args, create object for option parsing
	cxxopts::Options options("exma", "Analysis of extracelluar matrix and biofilm data");

	try {
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options.add_options("Input") //For all variables influencing the input
			("f,folder", "Folder name containing image data", cxxopts::value<std::string>(env->imageFolderName))
			("m,minimum_threshold", "Minimum intensity threshold for detection", cxxopts::value<int>(env->THRESHOLD)->default_value("50"))
			("conc_step", "Step size for concentration lines around 50%", cxxopts::value<int>(env->CONC_STEP)->default_value("20"))
			("layer_blur", "2D image blur radius", cxxopts::value<int>(env->LAYER_BLUR)->default_value("0"))
			("max_space", "Largest allowable vertical gap", cxxopts::value<int>(env->MAX_SPACE)->default_value("100"))
			;

		options.add_options("Analysis") //For all variables influencing the analysis
			("b,biofilm", "Biofilm thickness", cxxopts::value<bool>(env->A_BIOFILM))
			("c,concentration", "Concentration gradient", cxxopts::value<bool>(env->A_CONCENTRATION))
			("conc_offset", "Offset in pixels for start of concentration analysis from mixing point", cxxopts::value<int>(env->CONC_OFFSET)->default_value("200"))
			;

		options.add_options("Output") //For all variables influencing the output of data
			("s,save", "Save all outputs as images", cxxopts::value<bool>(env->SAVE))
			("t,table", "Save thickness vs concentration as table", cxxopts::value<bool>(env->TABLE))
			("o,overlay", "Overlay descriptive information on outputs", cxxopts::value<bool>(env->OVERLAY))
			("d,display", "Display all outputs to screen", cxxopts::value<bool>(env->DISPLAY))
			("disp_percent", "% scale of original image size", cxxopts::value<int>(env->DISP_PERCENT)->default_value("30"))
			;

		options.add_options() //Other options
			("h, help", "Print help")
			("v,verbose", "Verbose mode", cxxopts::value<bool>(env->VERBOSE))
			;

		auto result = options.parse(argc, argv);

		//If help is selected, do not continue to run the program and simply display the help text
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
	//Check if image folder exists
	if (!fs::exists(env->imageFolderName))
	{
		throw ("Image folder path " + env->imageFolderName + " does not exist").c_str();
	}

	int number_of_images = 0, number_of_non_images = 0;

	//Initialize vector of image names to be blank
	env->imageFileNames = {};

	for (auto & p : fs::directory_iterator(env->imageFolderName))
	{
		//Convert Path to string
		static std::string filename;
		filename = p.path().u8string();

		//Check if string represents a subfolder
		if (filename.find(".") == -1)
		{
			throw "Image folder must not contain subfolders";
		}

		//Set list of supported file types
		std::vector<std::string> supported_types;
		supported_types = { "bmp" , "BMP" };

		//Ensure the file is a supported filetype
		if (std::find(supported_types.begin(), supported_types.end(), filename.substr(filename.find_last_of(".") + 1)) != supported_types.end()) {
			//Add file to the image list
			env->imageFileNames.emplace_back(filename);
			number_of_images++;
		}
		else {
			number_of_non_images++;
		}
	}

	//Set the depth to be the number of images in the folder
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
			env->DISP_HEIGHT = env->HEIGHT*(float(env->DISP_PERCENT)*0.01f);
			env->DISP_WIDTH = env->WIDTH*(float(env->DISP_PERCENT)*0.01f);
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
			if (env->FACING && j < env->MIX_X)
			{
				if (OUTPUT[i][j] > maxVal)
				{
					maxVal = OUTPUT[i][j];
				}
			}
			if (!env->FACING && j > env->MIX_X)
			{
				if (OUTPUT[i][j] > maxVal)
				{
					maxVal = OUTPUT[i][j];
				}
			}
		}
	}

	env->MAX_THICKNESS = maxVal * float(env->DEPTH) / 255.f;

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
				static int M;
				M = 255 * float(OUTPUT[i][j]) / float(maxVal);
				if (M > 255)
					M = 255;

				output(j, i, 0) = M;
				output(j, i, 1) = 0;
				output(j, i, 2) = 255-M;
		}
	}

	return output;
}

cimg_library::CImg<unsigned char> calcConcentrationGradient(cimg_library::CImgList<unsigned char>* list, EnvironmentVariables* env)
{
	cimg_library::CImg<unsigned char> output = (*list)[1];
	int** OUTPUT;

	float C_max = 256.f*256.f*256.f, D = 100.f, h = float(env->HEIGHT) * env->PIXEL_WIDTH / 2.f, L = float(env->HEIGHT)* env->PIXEL_WIDTH ,flow_rate = env->FLOW_RATE*1000000000.f/(60.f*60.f*env->CROSS_AREA);
	int C = 0, k_max = 10;

	OUTPUT = new int*[env->HEIGHT];
	for (int i = 0; i < env->HEIGHT; i++)
	{
		OUTPUT[i] = new int[env->WIDTH];
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		if (env->FACING)
		{
			for (int j = env->WIDTH-1; j >= 0; j--)
			{
				if (j > env->MIX_X)
				{
					if (i > env->MIX_Y)
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
					for (int k = 1; k < k_max; k++)
					{
						sum_part += (1 / float(k))*
							std::sinf(float(k)*M_PI*h / L)*
							std::exp(-D * float(k)*float(k)*M_PI*M_PI*float(env->MIX_X + 1 - j)* env->PIXEL_WIDTH / (L* L*flow_rate))*
							std::cosf(float(k)*M_PI*(float(i - env->MIX_Y)* env->PIXEL_WIDTH + h) / L);
					}
					C = C_max * (sum_part * 2.f / M_PI + h / L);
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
		else
		{
			for (int j = 0; j < env->WIDTH; j++)
			{
				if (j < env->MIX_X)
				{
					if (i > env->MIX_Y)
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
					for (int k = 1; k < k_max; k++)
					{
						sum_part += (1 / float(k))*
							std::sinf(float(k)*M_PI*h / L)*
							std::exp(-D * float(k)*float(k)*M_PI*M_PI*float(j - env->MIX_X + 1)* env->PIXEL_WIDTH / (L* L*flow_rate))*
							std::cosf(float(k)*M_PI*(float(i - env->MIX_Y)* env->PIXEL_WIDTH + h) / L);
					}
					C = C_max * (sum_part * 2.f / M_PI + h / L);
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
	}

	for (int i = 0; i < env->HEIGHT; i++)
	{
		for (int j = 0; j < env->WIDTH; j++)
		{
			output(j, i, 0) = OUTPUT[i][j]/(256*256);
			output(j, i, 1) = (OUTPUT[i][j]/256)%256;
			output(j, i, 2) = OUTPUT[i][j]%256;
		}
	}

	return output;
}

void drawConcentrationLines(cimg_library::CImg<unsigned char> *concImage, cimg_library::CImg<unsigned char> *drawImage, EnvironmentVariables* env)
{
	//First, daw the concentration gradient lines for the different concentrations
	int draw_size = 4, textSize = 45;
	unsigned char white[] = { 255,255,255 };
	//Find smallest concentration stratification
	float step = 50.f, minstep;
	do
	{
		step = step - float(env->CONC_STEP);
	} while (step > 0.f);
	step += env->CONC_STEP;
	minstep = step;
	
	if (env->FACING)
	{
		for (int i = env->MIX_X - env->CONC_OFFSET; i >= 0; i--)
		{
			while (step < 100.f)
			{
				//Conc lowest on top
				if (getConcValue(concImage, 0, 0) < getConcValue(concImage, 0, env->HEIGHT - 1))
				{
					for (int j = 0; j < env->HEIGHT; j++)
					{
						if (getConcValue(concImage, i, j) > step)
						{
							if (i == env->MIX_X - env->CONC_OFFSET)
							{
								drawImage->draw_text(env->MIX_X - env->CONC_OFFSET + 60, j - textSize / 2, std::to_string(int(step)).c_str(), white, 0, 1, textSize);
							}
							step = step + float(env->CONC_STEP);
							drawAt(drawImage, i, j, 200, 200, 200, draw_size, env);
						}
					}
				}
				//Conc lowest on bottom
				else
				{
					for (int j = env->HEIGHT - 1; j >= 0; j--)
					{
						if (getConcValue(concImage, i, j) > step)
						{
							if (i == env->MIX_X - env->CONC_OFFSET)
							{
								drawImage->draw_text(env->MIX_X - env->CONC_OFFSET + 60, j - textSize / 2, std::to_string(int(step)).c_str(), white, 0, 1, textSize);
							}
							step = step + float(env->CONC_STEP);
							drawAt(drawImage, i, j, 200, 200, 200, draw_size, env);
						}
					}
				}
			}
			step = minstep;
		}
	}
	else
	{
		for (int i = env->MIX_X + env->CONC_OFFSET; i < env->WIDTH; i++)
		{
			while (step < 100.f)
			{
				//Conc lowest on top
				if (getConcValue(concImage, 0, 0) < getConcValue(concImage, 0, env->HEIGHT - 1))
				{
					for (int j = 0; j < env->HEIGHT; j++)
					{
						if (getConcValue(concImage, i, j) > step)
						{
							if (i == env->MIX_X + env->CONC_OFFSET)
							{
								drawImage->draw_text(env->MIX_X + env->CONC_OFFSET - 60, j - textSize / 2, std::to_string(int(step)).c_str(), white, 0, 1, textSize);
							}
							step = step + float(env->CONC_STEP);
							drawAt(drawImage, i, j, 200, 200, 200, draw_size, env);
						}
					}
				}
				//Conc lowest on bottom
				else
				{
					for (int j = env->HEIGHT - 1; j >= 0; j--)
					{
						if (getConcValue(concImage, i, j) > step)
						{
							if (i == env->MIX_X + env->CONC_OFFSET)
							{
								drawImage->draw_text(env->MIX_X + env->CONC_OFFSET - 60, j - textSize / 2, std::to_string(int(step)).c_str(), white, 0, 1, textSize);
							}
							step = step + float(env->CONC_STEP);
							drawAt(drawImage, i, j, 200, 200, 200, draw_size, env);
						}
					}
				}
			}
			step = minstep;
		}
	}

	//Draw legend for both distance and for height
	int LendX, LendY, LstartX, LstartY;
	if (env->FACING)
	{
		LendX = env->WIDTH - 50;
		LstartX = env->WIDTH - 50 - (50.f / env->PIXEL_WIDTH);
		LendY = env->HEIGHT - 100;
		LstartY = env->HEIGHT - 300;
	}
	else
	{
		LendX = 50 + (50.f / env->PIXEL_WIDTH);
		LstartX = 50;
		LendY = env->HEIGHT - 100;
		LstartY = env->HEIGHT - 300;
	}

		for (int i = 0; i < abs(LendX - LstartX); i++)
		{
			drawAt(drawImage, LstartX + i, LstartY, 255*float(i)/float(abs(LendX - LstartX)), 128, 255 - 255 * float(i) / float(abs(LendX - LstartX)), draw_size*3, env);
			drawAt(drawImage, LstartX + i, LendY, 255, 255, 255, draw_size*3, env);
		}
		std::string s;
		std::string ms = "Biofilm height (µm)";
		drawImage->draw_text(LstartX, LstartY - 55, ms.c_str(), white, 0, 1, textSize);
		drawImage->draw_text(LstartX, LstartY + 30 ,"0", white, 0, 1, textSize);
		drawImage->draw_text(LendX, LstartY + 30, std::to_string(env->MAX_THICKNESS).c_str(), white, 0, 1, textSize);
		drawImage->draw_text(LstartX, LendY - 55, "50 µm", white, 0, 1, textSize);

	for (int i = env->HEIGHT/4; i < 3*env->HEIGHT/4; i++)
	{
		if (env->FACING)
			drawAt(drawImage, env->MIX_X - env->CONC_OFFSET, i, 255, 50, 50, draw_size, env);
		else
			drawAt(drawImage, env->MIX_X + env->CONC_OFFSET, i, 255, 50, 50, draw_size, env);
	}
}

float getConcValue(cimg_library::CImg<unsigned char> *image, int x, int y)
{
	return 100.f*float(image->operator()(x, y, 0) * 256 * 256 + 256 * image->operator()(x, y, 1) + image->operator()(x, y, 2))/float(256*256*256);
}

void drawAt(cimg_library::CImg<unsigned char> *image, int x, int y, int R, int G, int B, int size, EnvironmentVariables* env)
{
	for (int i = -(size + 1) / 2; i < (size + 1) / 2; i++)
	{
		for (int j = -(size + 1) / 2; j < (size + 1) / 2; j++)
		{
			if (x + i >= 0 && y + j >= 0 && x + i < env->WIDTH &&  y + j < env->HEIGHT)
			{
				image->operator()(x + i, y + j, 0) = R;
				image->operator()(x + i, y + j, 1) = G;
				image->operator()(x + i, y + j, 2) = B;
			}
		}
	}
}

void calcBinnedThickness(cimg_library::CImg<unsigned char> *concImage, cimg_library::CImg<unsigned char> *biofilmImage, EnvironmentVariables* env)
{
	float binSize = 1.f;
	float* AV_THICKNESS;
	float* TOT_THICK;
	float* NUM_THICK;

	AV_THICKNESS = new float[(100.f / binSize)];
	TOT_THICK = new float[(100.f / binSize)];
	NUM_THICK = new float[(100.f / binSize)];

	for (int j = 0; j < int(100.f / binSize); j++)
	{
		AV_THICKNESS[j] = 0;
		TOT_THICK[j] = 0;
		NUM_THICK[j] = 0;
	}

	for (int i = env->MIX_X + env->CONC_OFFSET; i < env->WIDTH; i++)
	{
		for (int j = 0; j < env->HEIGHT; j++)
		{
			TOT_THICK[int(getConcValue(concImage, i, j) / binSize)] += float(biofilmImage->operator()(i, j, 0)) / 255.f*env->MAX_THICKNESS;
			NUM_THICK[int(getConcValue(concImage, i, j) / binSize)] += 1;
		}
	}

	for (int j = 0; j < int(100.f / binSize); j++)
	{
		if (NUM_THICK[j] > 0)
		{
			AV_THICKNESS[j] = TOT_THICK[j]/NUM_THICK[j];
		}
		else
		{
			AV_THICKNESS[j] = 0.f;
		}
	}

	std::ofstream outfile;
	outfile.open("Output.csv");
	outfile.clear();
	outfile << "% concentration upper stream, thickness (micrometers)\n";

	for (int j = 0; j < int(100.f / binSize); j++)
	{
		outfile << std::to_string(float(j*binSize)) << "," << std::to_string(AV_THICKNESS[j]) << "\n";
	}

	outfile.close();
}