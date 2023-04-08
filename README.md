# whatsOnFulGaz
**FulGaz** is a popular multi-platform virtual cycling app developed by Bizar Mobile. At the time of this writing, its library includes more than 2,000 routes in 56 different countries. A companion web applet https://whatsonfulgaz.com is used to interactively search the route library using a web browser.  

**whatsOnFulGaz** is a simple command-line tool similar in functionality to the homonymous web applet, but intended to be used mostly in unattended batch mode. 

The tool has the following features:

1. It can generate a comma-separated-value (CSV) file that can then be viewed with MS Excel, Google Sheets, LibreOfice Calc, or any other spreadsheet app.  The CSV file has an entry (row) for each matching ride, which includes the most relevant data about the ride: e.g. contributor, country, length, elevation gain, etc.  Once the CVS file is read by the spreadsheet app, the routes (rows) can be sorted based on any of its attributes (columns).

2. It can generate an HTML file that can then be viewed with any web browser.  The web page includes a table with the same info as in the CSV file.  Each ride includes the links to download the 720p, 1080p, or 4K video file of the ride.

3. It supports match filters to select rides that where filmed by a given contributor, in a given country, with a given title, etc.

4. It can automatically download the MP4 video file or the SHIZ control file of all the matching rides, into a selected download folder.

# Building the tool

**whatsOnFulGaz** is written entirely in C and only uses the well-known CURL library. The tool is known to build under Windows (Cygwin), macOS Ventura, Ubuntu 22.04, and Rocky Linux 9.
 
To build the **whatsOnFulGaz** tool all you need to do is clone the repo from GitHub and run 'make' at the top-level directory:

```
$ git clone https://github.com/elfrances/whatsOnFulGaz.git
$ cd whatsOnFulGaz
$ make
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -D__CYGWIN__ -o download.o -c download.c
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -D__CYGWIN__ -o json.o -c json.c
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -D__CYGWIN__ -o main.o -c main.c
cc -m64 -D_GNU_SOURCE -I. -ggdb -Wall -Werror -O0 -D__CYGWIN__ -o output.o -c output.c
cc -ggdb  -o ./whatsOnFulGaz ./download.o ./json.o ./main.o ./output.o -lcurl
```

If your OS is missing the CURL library, you'll get a compilation error like this:

```
main.c:12:10: fatal error: curl/curl.h: No such file or directory
```

How to install the CURL library depends on the OS you are using.

- When using Cygwin on Windows you need to install it via the Cygwin's "setup.exe" package management tool, and select the libcurl-devel, libcurl-doc, and libcurl4 packages, as shown [here](assets/cygwin_setup.png).
  
- When using Ubuntu you can install it using the "apt-get" command, as shown below:

```
$ sudo apt-get install libcurl4
$ sudo apt-get install libcurl4-openssl-dev
```

- When using Rocky Linux you can install it using the "apt-get" command, as shown below:

```
$ sudo yum install libcurl
$ sudo yum install libcurl-devel
```

# Usage

Running the tool with the --help argument will print the list of available options:

```
SYNTAX:
    whatsOnFulGaz [OPTIONS]

    whatsOnFulGaz is a command-line app that parses the JSON file that contains all
    the available rides in the FulGaz library, and creates a CSV, HTML, or TXT
    file with the list of routes. The CSV file can be viewed with a spreadsheet
    app such as MS Excel, Google Sheets, or LibreOffice Calc, while the HTML file
    can be viewed with a web browser app such as Chrome, Edge, or Safari.
    The TXT file shows the route info in plain human-readable format.
    The app has several filters that allow it to show only selected routes; e.g. by
    contributor, country, maximum distance, etc.
    Optionally, the app can download in the background the MP4 video file of all the
    routes that matched the specified filters.

OPTIONS:
    --allrides-file <path>
        Specifies the path to the JSON file that describes all the available
        rides in the library.
    --category <name>
        Only include rides from the specified category. The name match is
        case-insensitive and liberal: e.g. specifying "hill" will match
        all rides contained in the category "Hilly". FulGaz supports the
        following categories: Avatar, Easy, Hilly, IRONMAN, Long, Loop,
        Mountain, New, Race, Sightseeing, Trails.
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
    --dry-run
        Show what is going to be downloaded, without actually downloading
        anything.
    --get-shiz
        Download the SHIZ control file of the ride.
    --get-video {720|1080|4k}
        Download the MP4 video file of the ride at the specified resolution.
    --help
        Show this help and exit.
    --max-distance <value>
        Only include rides with a distance (in Km's) up to the specified
        value.
    --max-duration <value>
        Only include rides with a duration (in minutes) up to the specified
        value.
    --max-elevation-gain <value>
        Only include rides with an elevation gain (in meters) up to the
        specified value.
    --min-distance <value>
        Only include rides with a distance (in Km's) above the specified
        value.
    --min-duration <value>
        Only include rides with a duration (in minutes) above the specified
        value.
    --min-elevation-gain <value>
        Only include rides with an elevation gain (in meters) above the
        specified value.
    --output-format {csv|html|text}
        Specifies the format of the output file with the list of routes.
        If omitted, the plain text format is used by default.
    --province <name>
        Only include rides from the specified province or state in the
        specified country. The name match is case-insensitive and liberal:
        e.g. specifying "cali" will match all rides from California, USA.
    --title <name>
        Only include rides that have <name> in their title. The name
        match is case-insensitive and liberal: e.g. specifying "gavia"
        will match the rides "Passo di Gavia", "Passo di Gavia Sweet
        Spot", and "Passo di Gavia from Ponte di Legno".
    --version
        Show program's version info and exit.

NOTES:
    Running the tool under Windows/Cygwin the drive letters are replaced by
    their equivalent cygdrive: e.g. the path "C:\Users\Marcelo\Documents"
    becomes "/cygdrive/c/Users/Marcelo/Documents".

BUGS:
    Report bugs and enhancement requests to: marcelo_mourier@yahoo.com
```

# A note about running whatsOnFulGaz under Windows/Cygwin

When running the tool under Windows/Cygwin notice that the drive letters used in path names, such as C: and D:, are replaced by the corresponding "cygdrive".  For example, the path:

```
C:\Users\Marcelo\Documents
```

becomes: 

```
/cygdrive/c/Users/Marcelo/Documents
```

and the path: 

```
D:\FulGaz\Videos
```

becomes: 

```
/cygdrive/d/FulGaz/Videos
```

# Example 1

Generate a CSV file with all the rides in the library and store it in the file [AllRides.csv](assets/AllRides.csv.png) in the current folder.  This file can then be opened with Excel or with LibreOffice Calc, where the rides (rows in the spreadsheet) can be sorted by any of the available column values: e.g. contributor, country, length, etc.

```
$ ./whatsOnFulGaz --output-format csv > AllRides.csv
```

# Example 2

Generate an HTML file with all rides that have a distance of up to 20 km and an elevation gain of up to 100 meters, and store it in the file [EasyRides.html](assets/EasyRides.html.png) in the current folder:

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
    Province/State:  Friuli-Venezia Giulia
    Contributor:     Hans Peter Obwaller
    Categories:      Mountain
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
    Province/State:  Friuli-Venezia Giulia
    Contributor:     Hans Peter Obwaller
    Categories:      Mountain
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
    Province/State:  Friuli-Venezia Giulia
    Contributor:     Hans Peter Obwaller
    Categories:      Mountain
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

Download the SHIZ control file of each of the rides filmed by Hans Peter Obwaller, and store them in the folder D:\FulGaz\Shiz:

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

Download the 1080p video file of each of the rides filmed by Rob Bennett in France (showing the file download progress), and store them in the folder D:\FulGaz\Videos:

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

When using the **whatsOnFulGaz** tool to download the shiz and/or video files of a group of rides, and the download gets unexpectedly interrupted, you can simply re-run the tool to pick up where it left off, downloading only the remaining missing files, or the truncated files.  For example, the "Death Ride" challenge includes 5 rides, and assume the download gets interrupted while downloading the 4th ride "Ebbetts South Ascent". By re-running the tool, the first 3 rides are skipped, the 4th one is re-downloaded because it was truncated, and the 5th ride is downloaded as well, because it had not been downloaded during the first run: 

```
$ ./whatsOnFulGaz --category "death ride" --get-shiz --download-folder /cygdrive/d/FulGaz
INFO: Skipping file Monitor-Pass-West-Ascent-seg.shiz because it already exists in the specified download folder.
INFO: Skipping file Monitor-East-Ascent-seg.shiz because it already exists in the specified download folder.
INFO: Skipping file Ebbetts-North-NEW-seg.shiz because it already exists in the specified download folder.
Downloading: https://assets.fulgaz.com/Ebbetts-South-Ascent-seg.shiz ....
INFO: File Carson-East-Ascent-seg.shiz already exists in the specified download folder, but with a different size: url=852570 file=8577
Downloading: https://assets.fulgaz.com/Carson-East-Ascent-seg.shiz ....
```

# Example 7

When running the **whatsOnFulGaz** tool on a system where the official **FulGaz** app is installed, the tool can automatically figure out the location of the JSON file that contains the list of available rides.  However, when running the app on a system where the **FulGaz** app is not installed or is not supported (such as Ubuntu), one can still use the **whatsOnFulGaz** tool by manually specifying the location of the JSON file using the ``--allrides-file <path>`` option. This JSON file can be copied over from a Windows PC or Mac where the **FulGaz** app is installed.  

In Windows the path to this file is: 

``C:\Users\Marcelo\AppData\Local\Packages\BizarMobile.FulGaz_asxhtrxdca8p2\LocalCache\Local\FulGaz\allrides_v4.json``

And on macOS the path to the file is:

``/Users/Marcelo/Library/Containers/com.bizarmobile.fulgaz/Data/Library/Application Support/FulGaz/allrides_v4.json``

Of course, you'll have to replace ``Marcelo`` for your own Windows/macOS username in the above paths to find the file!

```
$ ./whatsOnFulGaz --allrides-file Downloads/allrides_v4.json --contributor mourier --title cuadrado
{
    Name:            Camino del Cuadrado
    Country:         Argentina
    Province/State:  Cordoba
    Contributor:     Marcelo Mourier
    Categories:      Hilly
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
    Province/State:  Cordoba
    Contributor:     Marcelo Mourier
    Categories:      Easy
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

# Example 8

To see what the tool is going to download, without actually downloading anything, you can use the ``--dry-run`` option:

```
$ ./whatsOnFulGaz --category "IRONMAN" --get-shiz --download-folder /cygdrive/d/FulGaz --dry-run
Would download: https://assets.fulgaz.com/kona-bike-2022-Avatar2.shiz ....
Would download: https://assets.fulgaz.com/IM-Nur-Sultan-70.3.shiz ....
Would download: https://assets.fulgaz.com/703-Acapulco-Bike.shiz ....
     .
     .
     .
Would download: https://assets.fulgaz.com/IM-Kona-Bike-Turnaround.shiz ....
Would download: https://assets.fulgaz.com/kona-2022-part3.shiz ....
Would download: https://assets.fulgaz.com/kona-2022-part4.shiz ....
```



