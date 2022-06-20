<h1 align="center">SeqPaint</h1>

<p align="center">A single header library to convert images to VT100 escape sequences</p>

## Features

* Simple to use : only one function
* Supports transparency
* Optional background colour
* Optional colour blending
* Custom character sets (supports UTF-8 unicode)
* Custom filters, area, character distribution, colouring

## How to Use

1. Download ```stb_image.h``` [here](https://github.com/nothings/stb/blob/master/stb_image.h), include in the same directory as ```seqpaint.h```.

2. Include ```seqpaint.h``` in your project.

## Documentation

### Function

```C
enum SeqPaintError getSequence(char const* file, char const* ret, struct SeqPaintParam const* param)
```

```file``` is the image you want to use. ```ret``` is the file the sequence will be put in. ```param``` is the list of options, or ```NULL``` if you want all defaults.

### Parameters

You **do not** have to include all the parameters. In C99, you can specify individual members; the unspecified ones will be the **default** values. For example:

```C
struct SeqPaintParam param = {
    .scaleX = 17,
    .scaleY = 24
};
```

| Parameters  | Description | Type | Default |
| ------------- | ------------- | ------------- | ------------- | 
| ```scaleX``` | X scale of image. Higher is smaller | ```int``` | ```1``` |
| ```scaleY``` | Y scale of image. Higher is smaller | ```int``` | ```1``` |
| ```filter``` | Filters image. One of ```SEQPAINT_FILTER_NONE```, ```SEQPAINT_FILTER_SEPIA```, ```SEQPAINT_FILTER_GRAYSCALE```, ```SEQPAINT_FILTER_INVERT```, or ```SEQPAINT_FILTER_CUSTOM```. Custom filter requires defining the ```SEQPAINT_FILTER_EXPR``` filter macro, where you can set ```col->r```, ```col->g```, ```col->b``` to values. | Read desc | ```SEQPAINT_FILTER_NONE``` |
| ```charLen``` | Length of character set | ```int``` | ```1``` |
| ```charSet``` | Character set; set of characters to make up image | ```char**``` | ```{" "}``` |
| ```charFunc``` | Function that determines where to put characters. Can be ```charBright``` (brighter colours will get higher set characters), ```charRand``` (random), and ```charDefault``` (first character of set), or you can pass in your own function in the form ```char* func(struct SeqPaintCol, int, char**)```| Read desc | ```charDefault``` |
| ```range``` | Number of characters to blend | ```int``` | ```0``` |
| ```blend``` | Blend surrounding colours (take average rgb values) according to range. One of ```SEQPAINT_BLEND_NONE```, ```SEQPAINT_BLEND_X``` (samples from x direction), ```SEQPAINT_BLEND_Y``` (samples from y), ```SEQPAINT_BLEND_XY``` (samples from x and y)| Read desc | ```SEQPAINT_BLEND_NONE``` |
| ```area``` | Area of image to convert in the form ```&(struct SeqPaintRect) {x, y, w, h}``` | Read desc | Entire image |
| ```alphaRep``` | Colour to replace transparency in the form ```&(struct SeqPaintCol) {r, g, b}```| Read desc | Transparent |
| ```disp``` | Type of colouring to use. One of ```SEQPAINT_DISP_BKG``` (colour background) or ```SEQPAINT_DISP_CHAR``` (colour characters) | Read desc | ```SEQPAINT_DISP_BKG``` |
| ```noConsole``` | Don't print to console if true | ```bool``` | ```false``` |
| ```appendFile``` | Append to file if true | ```bool``` | ```false```|

## Error Handling

```getSequence``` will return one of these errors.

| Error | Meaning |
| ------------- | ------------- |
| ```SEQPAINT_ERROR_NONE``` | No errors, all good! |
| ```SEQPAINT_ERROR_IMAGE``` | Image failed to load |
| ```SEQPAINT_ERROR_MEMORY``` | Cannot allocate memory for sequence |
| ```SEQPAINT_ERROR_COORD``` | ```area``` was not in bound |
| ```SEQPAINT_ERROR_FILE``` | File to write cannot be opened |
