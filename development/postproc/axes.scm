;
;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.
;
; Adds Axes to a plot
;
; Todo: make the ylabel 90degree orientation
;
( define (sciber-axes inFile outFile x0 x1 y0 y1 nx ny xLabel yLabel title xOffset yOffset fontSize fontType )
  (let* (
         (frameIm (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
         (frameDw (car (gimp-image-get-active-layer frameIm)))
         (imwc (car (gimp-image-width frameIm)))
         (imhc (car (gimp-image-height frameIm)))
         (fontSizePixels (* imhc fontSize ))
         (fs  0.25) 
         (fpx 10)
         (lm (+ fontSizePixels 8 ))     ;Left Margin;
         (tm 15 )                       ;Top margin
         (bm (+ fontSizePixels 8))      ;Bottom Margin
         (rm 14 )                       ;Right Margin
         fpx
         yInitial xInitial
         xAxisOffset yAxisOffset
         delta
         increment counter
         xPosition  yPosition
         displayNumber
         delta increment 
         )
    (set! fpx 10)
    (set! yLabelYPosition (/ imhc 2 ))
    (set! yLabelXPosition (/ lm 2 ))
    (set! xLabelYPosition (- imwc bm))
    (set! xLabelXPosition (/ imwc 2 ))
    
    (plug-in-autocrop RUN-NONINTERACTIVE frameIm frameDw)
    ;
    ; XAxis
    ;
    (set! yInitial  (- imhc (+ tm bm) ))
    (set! xInitial  lm )

    (set! xAxisOffset (floor (/ bm 2 )))
    (set! yAxisOffset (floor (/ lm 2 )))


    (set! delta   (floor (/ (- imwc (+ lm rm )) (- nx  1))))
    (set! increment (/ (- (string->number x1) (string->number x0 )) (- nx 1 )))
    (set! counter 0 )
    (set! xPosition xInitial )

    (while (< counter nx )
           (set! xPosition (+ xInitial (* counter delta )))
           (set! displayNumber  (+ (* counter increment ) (string->number x0 )))
           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xPosition (+ yInitial xAxisOffset) (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
           (set! counter (+ 1 counter ))
           );while
    ;
    ; Xlabel
    ;
    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xLabelXPosition xLabelYPosition xLabel 0 TRUE fontSizePixels PIXELS "Nimbus Mono L Bold Oblique")))

    ;
    ; Yaxis
    ;
    (set! delta   (floor (/ (- imwc (+ tm bm )) (- ny 1 ))))

    (set! increment (/ (- (string->number y1) (string->number y0 )) (- ny 1 )))
    (set! counter ny )
    (set! yPosition yInitial )
    (while (> counter 0 )
           (set! yPosition (- yInitial ( * ( - ny counter)  delta )))
           (set! displayNumber  (+ (* (- ny counter ) increment ) (string->number y0 )))
           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw  (floor (- xInitial yAxisOffset ))  yPosition (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
           (set! counter (- counter 1 ))
           );while

    ;
    ; Ylabel
    ;
;    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw yLabelXPosition yLabelYPosition yLabel 0 TRUE fontSizePixels PIXELS "Nimbus Mono L Bold Oblique")))
    (gimp-floating-sel-anchor (plugin-rotate (car (gimp-text-fontname frameIm frameDw yLabelXPosition yLabelYPosition yLabel 0 TRUE fontSizePixels PIXELS "Nimbus Mono L Bold Oblique")))   )

    ;
    ; Title
    ;
    (set! xTitle (/  imwc 2 ))
    (set! yTitle tm )
    (set! fpTitle  20 )
    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw  xTitle  yTitle title 0 TRUE fpTitle PIXELS "Nimbus Mono L Bold Oblique")))

    (gimp-displays-flush )
    (gimp-file-save RUN-NONINTERACTIVE frameIm frameDw outFile outFile)
    );let*
);define


(script-fu-register "sciber-axes" 
                    "Axes"
                    "Automatically adds axes to an image "
                    "Jimi Damon <jdamon@gmail.com>"
                    "Jimi Damon"
                    "2010-06-08"
                    ""
                    SF-STRING "InputFile" "0"
                    SF-STRING "Output File" "0"
                    SF-STRING "xInitial (x0) as a string" "0"
                    SF-STRING "xFinal   (xF) as a string" "10"
                    SF-STRING "yInitial (y0) as a string" "0"
                    SF-STRING "yFinal   (yF) as a string" "0"
                    SF-STRING "Tile" "TITLE"
                    SF-VALUE "Xoffset for the yaxis tick marks" "10"
                    SF-VALUE "Yoffset for the xaxis tick marks" "10"
                    SF-VALUE "Font Size for Label as a %" "15"
                    SF-STRING "Fond Type"  "San"
                    )
(script-fu-menu-register "sciber-crop" "<Toolbox>/Xtns/Script-Fu/SciberQuest" )


(sciber-axes  "/home/jdamon/test.png" "/home/jdamon/foo2.png" "2" "10" "0" "10" 5 15  "xlabel" "ylabel" "NEW TITLE" 12 13  0.04 "San" )


;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
; Helper functions
;=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

(define (add-message message x y ) 
  (let*(()
        (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw x y message 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
        )
    )
);define


(define (ceiling  x ) 
  (+ x (- 1 (fmod x 1)))
);

(define (floor  x ) 
  (- x (fmod x 1))
);


(define (font-pixelsize-from-percent-size image )
  (2 )
);define
