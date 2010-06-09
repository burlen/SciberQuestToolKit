;
;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.
;
;
(define (sciber-annotate inFile outFile label fontSizePercent x y fonttype )
  (let* ((image (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile )))
         (drawable (car (gimp-image-get-active-layer image)))
         (fontSizePixel (* fontSizePercent (car (gimp-image-height image))))
         text-float
         )
    (set! text-float (car (gimp-text-fontname image drawable
                                              x y label 0 1 fontSizePixel 0
                                              fonttype )))
    (gimp-floating-sel-anchor text-float)
    (gimp-displays-flush)
    (gimp-file-save RUN-NONINTERACTIVE image drawable outFile outFile )
  );let
);define

(script-fu-register "sciber-annotate" 
                    "Annotate"
                    "Annotates an image with text of a custom font, size and position"
                    "Jimi Damon <jdamon@gmail.com>"
                    "Jimi Damon"
                    "2010-06-08"
                    ""
                    SF-STRING "InputFile" "0"
                    SF-STRING "Output File" "0"
                    SF-STRING "Label" "TEST"
                    SF-VALUE  "fontSize" "0.10"
                    SF-VALUE  "xCoord"   "0"
                    SF-VALUE  "yCoord"   "0"
                    SF-STRING  "fontType"  "Sans")
(script-fu-menu-register "sciber-annotate" "<Toolbox>/Xtns/Script-Fu/SciberQuest" )
