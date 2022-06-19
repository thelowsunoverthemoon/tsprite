<h1 align="center">SeqPaint</h1>

<p align="center">A single header library to convert images to VT100 escape sequences</p>

## Features

* Simple to use : only one function
* Supports transparency
* Optional background colour
* Optional colour blending
* Custom character sets, filters, area

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

You do not have to include all the parameters. In C99, you can specify individual members; the unspecified ones will be the default values. For example:

```C
struct SeqPaintParam param = {
    .scaleX = 17,
    .scaleY = 24
};
```

| Parameters  | Description | Type | Default |
| ------------- | ------------- | ------------- | ------------- | 
| ```scaleX``` | | | |
| ```scaleY``` | | | |
| ```filter``` | | | |
| ```charLen``` | | | |
| ```charSet``` | | | |
| ```charFunc``` | | | |
| ```range``` | | | |
| ```blend``` | | | |
| ```area``` | | | |
| ```alphaRep``` | | | |
| ```noConsole``` | | | |
| ```appendFile``` | | | |

## Error Handling
