# whatsOnFulGaz
FulGaz is a popular multi-platform virtual cycling app. At the time of this writing, its library includes more than 2,000 routes in 56 different countries. A companion web applet https://whatsonfulgaz.com is used to interactively browse the route library.  **whatsOnFulGaz** is a simple command-line tool similar in functionality to the web applet, but intended to be used mostly in unattended batch mode. 

The tool has the following features:

1. It can generate a comma-separated-value (CSV) file that can then be viewed with Excel, LibreOfice Calc, or any other spreadsheet app.  The CSV file has an entry (row) for each matching ride, which includes the most relevant data about the ride: e.g. contributor, country, length, elevation gain, etc.

2. It can generate an HTML file that can then be viewed with any web browser.  The web page includes a table with the same info as in the CSV file.  Each ride includes the links to download the 720p, 1080p, or 4K video file of the ride.

3. It supports match filters to select rides that where filmed by a given contributor, in a given country, with a given title, etc.

4. It can automatically download the MP4 video file or the SHIZ control file of all the matching rides, into a selected download folder.

# Building the tool

To build the **whatsOnFulGaz** tool all you need to do is run 'make' at the top-level directory. The tool is known to build under Windows (Cygwin), macOS Ventura, and Ubuntu. As it is written entirely in C and only uses the well-known CURL library, it should be easy to port to other platforms as well.

```
$ make
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -o json.o -c json.c
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -o main.o -c main.c
cc -ggdb  -o ./whatsOnFulGaz ./json.o ./main.o -lcurl
```

If your OS is missing the CURL library, you'll get a compilation error like this:

```
main.c:12:10: fatal error: curl/curl.h: No such file or directory
```

How to install the CURL library depends on the OS you are using.  In the case of Windows (Cygwin) you need to install it via the Cygwin's "setup.exe" package management tool.  In the case of Ubuntu you can install it using the following command:

```
$ sudo apt-get install libcurl4
$ sudo apt-get install libcurl4-openssl-dev
```

# Usage

Running the tool with the --help argument will print the list of available options:

```
$ ./whatsOnFulGaz --help
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
    --output-format {csv|html|text}
        Specifies the format of the output file with the list of routes.
        If omitted, the plain text format is used by default.
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
$ ./whatsOnFulGaz --output-format csv > AllRides.csv
```

# Example 2

Generate an HTML file with all rides that have a distance of up to 20 km and an elevation gain of up to 100 meters, and store it in the file EasyRides.html:

```
$ ./whatsOnFulGaz --output-format html --max-distance 20 --max-elevation-gain 100 > EasyRides.html
```

# Example 3

Show all rides that have the word "zoncolan" in their title:

```
$ ./whatsOnFulGaz --title zoncolan
{
    Name:            Monte Zoncolan Priola
    Country:         Italy
    Contributor:     Hans Peter Obwaller
    Distance:        10.87
    Elevation Gain:  1175
    Duration:        01:00:14
    Toughness Score: 616
    720p Video:      https://fulgaz.cachefly.net/file/fulgaz-videos/720P/Monte-Zoncolan-Priola.mp4
    1080p Video:     https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Monte-Zoncolan-Priola.mp4
    4K Video:        https://fulgaz.cachefly.net/file/fulgaz-videos/4K/Monte-Zoncolan-Priola.mp4
    SHIZ:            https://assets.fulgaz.com/Monte-Zoncolan-Priola-seg.shiz
}
{
    Name:            Zoncolan from Ovaro
    Country:         Italy
    Contributor:     Hans Peter Obwaller
    Distance:        10.70
    Elevation Gain:  1176
    Duration:        00:59:03
    Toughness Score: 615
    720p Video:      https://fulgaz.cachefly.net/file/fulgaz-videos/720P/Ovaro-Zoncolan.mp4
    1080p Video:     https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Ovaro-Zoncolan.mp4
    4K Video:        https://fulgaz.cachefly.net/file/fulgaz-videos/4K/Ovaro-Zoncolan.mp4
    SHIZ:            https://assets.fulgaz.com/Ovaro-Zoncolan-seg.shiz
}
{
    Name:            Zoncolan from Sutrio
    Country:         Italy
    Contributor:     Hans Peter Obwaller
    Distance:        13.38
    Elevation Gain:  1154
    Duration:        00:59:26
    Toughness Score: 513
    720p Video:      https://fulgaz.cachefly.net/file/fulgaz-videos/720P/Zoncolan-Sutrio.mp4
    1080p Video:     https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Zoncolan-Sutrio.mp4
    4K Video:        https://fulgaz.cachefly.net/file/fulgaz-videos/4K/Zoncolan-Sutrio.mp4
    SHIZ:            https://assets.fulgaz.com/Zoncolan-Sutrio-seg.shiz
}
```

# Example 4

Download the SHIZ control file of each of the rides filmed by Hans Peter Obwaller, and store them in the D:\FulGaz\Shiz folder:

```
$ ./whatsOnFulGaz --contributor obwaller --get-shiz --download-folder /cygdrive/d/FulGaz/Shiz/
Downloading: https://assets.fulgaz.com/Above-Uttendorf-working-seg.shiz ....
Downloading: https://assets.fulgaz.com/Alpenverein.shiz ....
Downloading: https://assets.fulgaz.com/Alpenvereinshutte-seg.shiz ....
     .
     .
     .
```

# Example 5

Download the 1080p video file of each of the rides filmed by Rob Bennett in France (showing the file download progress), and store them in the D:\FulGaz\Videos folder:

```
$ ./whatsOnFulGaz --contributor bennett --country france --get-video 1080 --download-progress --download-folder /cygdrive/d/FulGaz/Videos/
Downloading: https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Tour-De-France-2022-Stage-20-Rocamadour-Time-Trial.mp4 ....
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  3 4374M    3  170M    0     0  60.0M      0  0:01:12  0:00:02  0:01:10 60.0M
     .
     .
     .
```

# Example 6

When running the **whatsOnFulGaz** tool on a system where the FulGaz app is installed, the tool can automatically figure out the location of the JSON file that contains the list of available rides.  However, when running the app on a system where the FulGaz app is not installed or not supported (such as Ubuntu), one can manually specify the location of the JSON file using the ``--allrides-file <path>`` option:

```
$ ./whatsOnFulGaz --allrides-file Downloads/allrides_v4.json --contributor mourier --title cuadrado
{
    Name:            Camino del Cuadrado
    Country:         Argentina
    Contributor:     Marcelo Mourier
    Distance:        19.02
    Elevation Gain:  653
    Duration:        01:09:40
    Toughness Score: 382
    720p Video:      https://fulgaz.cachefly.net/file/fulgaz-videos/720P/Camino-Del-Cuadrado.mp4
    1080p Video:     https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Camino-Del-Cuadrado.mp4
    4K Video:        https://fulgaz.cachefly.net/file/fulgaz-videos/4K/Camino-Del-Cuadrado.mp4
    SHIZ:            https://assets.fulgaz.com/Camino-Del-Cuadrado-working-seg.shiz
}
{
    Name:            Camino del Cuadrado Downhill
    Country:         Argentina
    Contributor:     Marcelo Mourier
    Distance:        17.82
    Elevation Gain:  105
    Duration:        00:30:47
    Toughness Score: 63
    720p Video:      https://fulgaz.cachefly.net/file/fulgaz-videos/720P/Camino-Del-Cuadrado-Downhill.mp4
    1080p Video:     https://fulgaz.cachefly.net/file/fulgaz-videos/1080P/Camino-Del-Cuadrado-Downhill.mp4
    4K Video:        https://fulgaz.cachefly.net/file/fulgaz-videos/4K/Camino-Del-Cuadrado-Downhill.mp4
    SHIZ:            https://assets.fulgaz.com/Camino-Del-Cuadrado-Downhill-working-seg.2.shiz
}
```


