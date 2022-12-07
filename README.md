# whatsOnFulGaz
A simple command-line tool similar in functionality to the web applet https://whatsonfulgaz.com, but intended to be used mostly in unattended batch mode. The tool has the following features:

1. It can generate a comma-separated-value (CSV) file that can then be viewed with Excel, LibreOfice Calc, or any other spreadsheet app.  The CSV file has an entry (row) for each matching ride, which includes the most relevant data about the ride: e.g. contributor, country, length, elevation gain, etc.

2. It can generate an HTML file that can then be viewed with any web browser.  The web page includes a table with the same info as in the CSV file.  Each ride includes the links to download the 720p, 1080p, or 4K video file of the ride.

3. It supports match filters to select rides that where filmed by a given contributor, in a given country, etc.

4. It can automatically download the MP4 video file of all the matching rides, into a selected download folder.

# Building the tool

To build the whatsOnFulGaz tool all you need to do is run 'make' at the top-level directory. The tool is known to build under Windows (Cygwin) and macOS. As it is written entirely in C and only uses the well-known CURL library, it should be easy to port to other platforms.

```
$ make
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -o json.o -c json.c
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -o main.o -c main.c
cc -ggdb  -o ./whatsOnFulGaz ./json.o ./main.o -lcurl
```

# Usage

Running the tool with the --help argument will print the list of available options:

```
$ ./whatsOnFulGaz.exe --help
SYNTAX:
    whatsOnFulGaz [OPTIONS]

    This command-line utility parses the JSON file that describes all the
    available rides, and creates a CSV file or an HTML file with the list
    of routes, that can be viewed with Excel or LibreOffice Calc (CSV) or
    with Chrome or Edge (HTML).

OPTIONS:
    --contributor <name>
        Only include rides submitted by the specified contributor. The name
        match is case-insensitive and liberal: e.g. specifying "mourier"
        will match all rides contributed by "Marcelo Mourier".
    --country <name>
        Only include rides from the specified country. The name match is
        case-insensitive and liberal: e.g. specifying "aus" will match
        all rides from "Australia" and from "Austria".
    --download-folder <path>
        Specifies the folder where the downloaded files are stored.
    --download-progress
        Show video download progress info.
    --get-shiz
        Download the SHIZ control file of the ride.
    --get-video {720|1080|4k}
        Download the MP4 video file of the ride at the specified resolution.
    --help
        Show this help and exit.
    --max-distance <value>
        Only include rides with a distance (in Km's) up to the specified
        value.
    --max-elevation-gain <value>
        Only include rides with an elevation gain (in meters) up to the
        specified value.
    --output-format {csv|html}
        Specifies the format of the output file with the list of routes.
        If omitted, the CSV format is used by default.
    --title <name>
        Only include rides that have <name> in their title. The name
        match is case-insensitive and liberal: e.g. specifying "gavia"
        will match the rides "Passo di Gavia", "Passo di Gavia Sweet
        Spot", and "Passo di Gavia from Ponte di Legno".
    --version
        Show program's version info and exit.
```

# Example 1

Generate a CSV file with all the rides in the library and store it in the file AllRides.csv in the current folder.  This file can then be opened with Excel or with LibreOffice Calc, where the rides (rows in the spreadsheet) can be sorted by any of the available column values: e.g. contributor, country, length, etc.

```
$ ./whatsOnFulGaz.exe --output-format csv > AllRides.csv
```

# Example 2

Generate an HTML file with all rides that have a distance of up to 20 km and an elevation gain of up to 100 meters:

```
$ ./whatsOnFulGaz.exe --output-format html --max-distance 20 --max-elevation-gain 100 > EasyRides.html
```

# Example 3

Download the 1080p video file of each of the rides filmed by Rob Bennett in France, and store them in the D:\FulGaz\Videos folder:

```
$ ./whatsOnFulGaz.exe --contributor bennett --country france --get-video 1080 --download-folder /cygdrive/d/FulGaz/Videos/
```


