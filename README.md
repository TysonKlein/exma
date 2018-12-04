# exma
## **ex**tracellular **m**atrix **a**nalyzer - For simple analysis of biofilm and extracellular matrix data

exma is a command line program for analysis of biofilm thickness in static and microfluidics experiments. In addidtion to supplying the thickness of a series of images representing 3D data from an experiment, exma also stratifies thickness of the biofilm by concentration of top stream vs. bottom stream (for microfluidics mixing eperiments).

A biofilm, as interpreted by exma, can be represented with a series of images. The example set of images below is included in this repo.
These images all represent a vertical layer of a 3D snapshot for an experiment. Whether measuring the extracellular matrix or the individual cells, exma is able to stack these images, fill in gaps, and provide a custom estimate of the biofilm thickness.

![Biofilm Layers](Readme/biofilm_explain.png)

For microfluidics mixing experiments, exma can also calculate the concentration gradient of two fluids based on some constant data, then supply a thickness vs. concentration table for further analysis.

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
  -c, --concentration    Concentration gradient
      --conc_step arg    Step size for concentration lines around 50%
                         (default: 20)
      --conc_offset arg  Offset in pixels for start of concentration analysis
                         from mixing point (default: 200)

 Input/Output options:
  -f, --folder arg        Folder name containing image data
  -s, --save              Save all outputs as images
  -t, --table             Save thickness vs concentration as table
      --table_bins arg    Number of bins for output table (default: 500)
  -o, --overlay           Overlay descriptive information on outputs
  -d, --display           Display all outputs to screen
      --disp_percent arg  % scale of original image size (default: 30)
```

## Basic functions

Note that all options with **arg** must be followed by a space, then the argument, then another space.

ex: **-m 51** sets the minimum threshold to 51

# Biofilm Options
*-m arg or --minimum_threshold arg* : Sets arg to be the minimum threshold for detecting whether a pixel "counts", and should therefore be included in the thickness. This is set to 50 unless changed.

