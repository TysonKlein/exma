#include <iostream> // for printing and reading user input if needed
#include <experimental/filesystem> //For file manipulation
#include <string>
#include "CImg.h"

namespace fs = std::experimental::filesystem;

struct EnvironmentVariables //This struct will hold some important environment values between contexts
{
	int HEIGHT, WIDTH, DEPTH;
	std::string imageFolderName;
};

void IMGreader(const std::string imageName, int current_depth, int*** DATA, EnvironmentVariables* env);

int main(int ac, char* av[])
{
	//order of variables:
	//exma IMAGE_FOLDER[1]
	EnvironmentVariables env;

	for (int i = 1; i < ac; i++)
	{
		//Set variables from args
		switch (i)
		{
		case 1:
			env.imageFolderName = av[i];
			break;
		}
		std::cout << i << "=" << av[i] << "\n";
	}

	std::string imageName;
	cimg_library::CImgList<unsigned char> imageList;
	int totalImages = 0, currentImage = 0;

	for (auto & p : fs::directory_iterator(env.imageFolderName))
	{
		std::cout << p << std::endl;
		imageName = p.path().u8string();
		const char *charImageName = imageName.c_str();
		cimg_library::CImg<unsigned char> src(charImageName);

		imageList.insert(src);
		totalImages++;
	}

	env.HEIGHT = imageList.back().height();
	env.WIDTH = imageList.back().width();
	env.DEPTH = totalImages;

	cimg_library::CImg<unsigned char> output = imageList[1];
	int** DATA;

	DATA = new int*[env.HEIGHT];
	for (int i = 0; i < env.HEIGHT; i++)
	{
		DATA[i] = new int[env.WIDTH];
	}
		for (int i = 0; i < env.HEIGHT; i++)
		{
			for (int j = 0; j < env.WIDTH; j++)
			{
				DATA[i][j] = 0;
			}
		}

	int maxVal = 0;
	int threshold = 40;

	for (int i = 0; i < env.HEIGHT; i++)
	{
		for (int j = 0; j < env.WIDTH; j++)
		{
			for (int RGB = 0; RGB < 3; RGB++)
			{
				int bottom = -1, top = -1;
				for (int im = 0; im < totalImages; im++)
				{
					if ((int)imageList[im](j, i, 0, RGB) > threshold)
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
						DATA[i][j] += 255/totalImages;
					}
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
				for (int im = 0; im < totalImages; im++)
				{
					output(j, i, RGB) = DATA[i][j];
				}
			}
		}
	}

	cimg_library::CImgDisplay main_disp(output, "Click a point");

	main_disp.resize(500, 500, true);

	while (!main_disp.is_closed()) {
		main_disp.wait();
	}

	std::cout << env.DEPTH << std::endl;

	try {
		//IMGreader(blah blah blah);
	} catch (const char* msg) {
		std::cerr << msg << std::endl;
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