# exma
## extracellular matrix analyzer

**exma** is a Windows command line program for analysis of biofilms in both static and dynamic experiments. In addition to supplying the biofilm thickness, exma can also stratify thickness of the biofilm by concentration of top stream vs. bottom stream in dual-inlet microfluidics experiments.

exma works with a confocal image stack. Each image in a stack represents a vertical layer (z-plane) of a biofilm. Whether measuring the extracellular matrix or the individual cells, exma is able to analyze these images, fill in gaps, and provide a customizable estimate of the biofilm thickness. The example set of images below is included in this repo.

<img src="Readme/biofilm_explain.png" width="400" height="400">

For dual-inlet microfluidics experiments, exma can also calculate the concentration gradient of two fluids in a channel, then supply a biofilm thickness vs. concentration table for further analysis. This gradient can be combined with the thickness data to produce a descriptive image like the one shown below.

<img src="https://github.com/TysonKlein/exma/blob/master/Readme/exma-output.png" width="800" height="800">

exma requires experimental information including the diffusivity coefficient between the two liquids, the flow rate, the pixel size, the distance between z-plane layers, the cross-sectional area, and the channel width. 

The following was printed from the help file of exma:

```
Usage:
  exma [OPTION...]

  -h, --help     Print help
  -v, --verbose  Verbose mode

 Biofilm options:
  -m, --minimum_threshold arg  Minimum intensity threshold for detection
                               (default: 50)
      --layer_blur arg         2D image blur radius (default: 0)
      --max_space arg          Largest allowable vertical gap (default: 100)

 Concentration options:
  -c, --concentration      Concentration gradient
      --conc_step arg      Step size for concentration lines around 50%
                           (default: 20)
      --conc_offset arg    Offset in pixels for start of concentration
                           analysis from mixing point (default: 200)
      --conc_fidelity arg  Number of iterations to calculate concentration
                           gradient (default: 30)

 Input/Output options:
  -f, --folder arg        Folder name containing image data
  -s, --save              Save all outputs as images
  -t, --table             Save thickness vs concentration as table
      --table_bins arg    Number of bins for output table (default: 500)
  -o, --overlay           Overlay descriptive information on outputs
  -d, --display           Display all outputs to screen
      --disp_percent arg  % scale of original image size (default: 30)
```

## Command line options

Note that all options with **arg** must be followed by a space, then the argument, then another space.

ex: **-m 51** sets the minimum threshold to 51

**--minimum_threshold 51** does the same

### Biofilm Options
*-m arg or --minimum_threshold arg* : Sets arg to be the minimum threshold for detecting whether a pixel "counts", and should therefore be included in the thickness. Pixels have a brightness range between 0-255, and the threshold is set to 50 unless changed.

*--layer_blur arg* : For each horizontal layer (each image) set the pixel blur radius. This is set to 0 unless changed (no blur), and should only be changed if image data has 'dead pixels' that can be smoothed out.

*--max_space arg* : Sets the maximum amount of 'empty vertical space' that will be counted between two detected pixels

  ex: for **--max_space 5**, where we have 10 layers as below ( | represents a detected pixel, . is undetected)
  
  |........| = ||........ <- Thickness  = 2
  
  |....|.... = ||||||.... <- Thickness  = 6
  
### Concentration Options
*-c or --concentration* : Include this option to also calculate the concentration gradient for mixing between two streams in a microfluidics experiment. An image prompt with the bottom image layer will be provided so you can select the 'mixing point'.
  
*--conc_step arg* : Sets the step size for concentration lines in the output image. The concentration lines are 50% +/- conc_step until 0% or 100% is reached. This value is 20 unless changed.
  
  ex: for **--conc_step 20**, there will be concentration lines of 10%, 30%, 50%, 70% and 90% in the output image.
 
*--conc_offset arg* : Set the number of pixels downstream where all concentration calculations will begin from. This is the red line in the output image. This value is 200 pixels unless changed.

*--conc_fidelity arg** : Sets the number of iterations to calculate the concentration value of each pixel. The concentration calculation is an iterative function: the more iterations, the more accurate the concentration. This value is 30 unless changed, and gives accurate output for most scenarios.

### Input/Output options
*-f arg or --folder arg* : Name of the folder containing the confocal image stack. Ensure that the images are alphabetically ordered with the bottom layer first. A folder will be produced based off this folder name with exma output files, if any.

*-s or --save* : If included, this will save the output image (colourful one above) as well as a monochrome image representing just the biofilm thickness in the output folder.

*-t or --table* : If included, this will stratify and export biofilm thickness in μm vs concentration of top stream as a csv file. exma finds the average biofilm thickness for a range of percentages (bin size), and exports each average with its corresponding bin (lower bound).

  ex:
  
| Thickness (μm) | Concentration % |
|----------------|-----------------|
| 3.2            | 0               |
| 2.8            | 20              |
| 2.2            | 40              |
| 2.4            | 60              |
| 3.1            | 80              |

This table shows that the average biofilm thickness between 0%-20% top stream concentration is 3.2 μm.

*--table_bins arg* : This value is the number of total bins to be used for the above table. The above table is 5 bins, but images with large dimensions allow for much more granular bin sizes. The default value is 500 bins (0.2%).

*-o or --overlay* : If included, this will print descriptive information on the output image such as concentration lines, concentration offset line, 50μm scale and height to colour scale.

*-d or --display* : If included this will show the output image on screen.

*--disp_percent arg* : Sets the percent zoom of the display images. Since images are usually large, the default value is 30, representing 30% full size display images.
