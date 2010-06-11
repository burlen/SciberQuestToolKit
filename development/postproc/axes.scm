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

( define (xaxis-offset )


);define



( define (sciber-axes inFile outFile x0 x1 y0 y1 nx ny xLabel yLabel title xOffset yOffset fontSize fontType )
  (let* (
         (frameIm (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
         (frameDw (car (gimp-image-get-active-layer frameIm)))
         (imwc (car (gimp-image-width frameIm)))
         (imhc (car (gimp-image-height frameIm)))
         (fs  0.25) 
         (fpx 10)
         (lm 50 )
         (bm 50 ) 
         (rm 50 ) 
         (tm 50 )
         (th 50 )
         (tw 50 )
         fs fpx
         yInitial xInitial
         xAxisOffset yAxisOffset
         delta
         increment counter
         xPosition  yPosition
         displayNumber
         delta increment 
         )
    (set! fs  0.25) 
    (set! fpx 10)

    (set! originalWidth (car (gimp-image-height  frameIm )))
    (set! originalHeight (car (gimp-image-width  frameIm )))
;    (gimp-message (string-append "Original width: " (number->string originalWidth )))

;    (gimp-message (string-append "Original Height is : " (number->string imhc )))
    
    (set! frameDw (car (gimp-image-get-active-layer frameIm )))

    (plug-in-autocrop RUN-NONINTERACTIVE frameIm frameDw)

    (set! imhc (car (gimp-drawable-height frameDw)))
    (set! imwc (car (gimp-drawable-width frameDw)))

                                        ; resize
    (gimp-context-set-background '(255 255 255))
    (gimp-context-set-foreground '(  0   0   0))

    ; resize the image

;    (gimp-message (string-append "Resizing to "  (number->string (+ imwc lm rm))))
    (gimp-image-resize frameIm (+ imwc lm rm) (+ imhc tm bm) lm tm)
    (gimp-layer-resize frameDw (+ imwc lm rm) (+ imhc tm bm) lm tm)


    (set! imcx (+ lm (/ imwc 2)))
    (set! imcy (+ tm (/ imhc 2)))


    (set! newWidth  (gimp-image-width frameIm ))
    (set! newHeight (gimp-image-width frameIm ))
    
    
    (set! imwc (car (gimp-image-width  frameIm )))
    (set! imhc (car (gimp-image-height frameIm )))

    ;
    ; XAxis
    ;
    (set! yInitial  (- imhc (+ tm bm) ))
    (set! xInitial  lm )

    (set! xAxisOffset (floor (/ bm 2 )))
    (set! yAxisOffset (floor (/ lm 2 )))


;    (set! delta   (floor (/ (- imwc (+ lm rm )) (- nx  1))))
    (set! delta   (floor (/ (- imwc (+ lm rm )) (- nx  1))))
    (set! increment (/ (- (string->number x1) (string->number x0 )) (- nx 1 )))
    (set! counter 0 )
    (set! xPosition xInitial )
    
;    (gimp-message (string-append "New width: " (number->string imwc )
;                                 "Delta: " (number->string delta )
;                                 "Width: " (number->string (- imwc lm rm ))
;                                 "Increment :" (number->string increment )
;                                 "xInitial: " (number->string xInitial )
;                  );string-append
;     );
                                 
;    (gimp-message (string-append "Increment : " (number->string increment )))
;    (gimp-message (string-append "xInitial  : " (number->string xInitial )))
;    (gimp-message (string-append "The new height of the image is : " (number->string imhc) ))

    (set! xAxisYValue (+ yInitial xAxisOffset))
    (set! xAxisYValue (+ xAxisYValue 25 ))
;    (gimp-message (string-append "Values " (number->string xAxisYValue)))
;    (gimp-message (string-append "Axis should be " (number->string lm )))
;    (while (< counter 1 )
;    (gimp-message (string-append "Initiali X " (number->string xInitial )))
;    (gimp-message (string-append "Final should be :" (number->string 
;                                                      ( + xPosition (* (- nx 1 ) delta)))
;                  )
;    )

    (while (< counter nx )
           (set! xPosition (+ xInitial (* counter delta )))
           (set! displayNumber  (+ (* counter increment ) (string->number x0 )))
           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xPosition xAxisYValue (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))


           ;
           ; Draw the ticks
           (set! counter (+ 1 counter ))
     );while

;    (gimp-message (string-append "Final X Position" (number->string xPosition )))

    ;
    ; Xlabel
    ;
    (set! xLabelYPosition (- imwc (/ bm 4 )))
    (set! xLabelXPosition (/ imwc 2 ))
    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xLabelXPosition xLabelYPosition xLabel 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))

    ;
    ; Yaxis
    ;
    (set! delta   (floor (/ (- imwc (+ tm bm )) (- ny 1 ))))
    (set! increment (/ (- (string->number y1) (string->number y0 )) (- ny 1 )))
    (set! counter ny )
    (set! yInitial (+ yInitial tm ))
    (set! yPosition yInitial )
    (set! yAxisXValue (floor (- xInitial yAxisOffset )))

;    (gimp-message (string-append "Yinitial value is : " (number->string yPosition )))

    (while (> counter 0 )
;           (set! yPosition (+ (- yInitial ( * ( - ny counter)  delta ))) 7 )

           (set! yPosition (- yInitial ( * ( - ny counter)  delta )))
           (set! yPosition (+ yPosition 10 ))


           (set! displayNumber  (+ (* (- ny counter ) increment ) (string->number y0 )))

;           (gimp-message (string-append "Adding Ylabel at : " (number->string yPosition )))

           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw yAxisXValue (- yPosition 20) (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
           (set! counter (- counter 1 ))

     );while

;           (gimp-message (string-append "Initial Y is : " (number->string yPosition )))
;           (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw (floor (- xInitial yAxisOffset ))   yPosition (number->string displayNumber) 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))


    ;
    ; Ylabel
    ;
    (set! yLabelYPosition (/ imhc 2 ))
    (set! yLabelXPosition (/ lm 2 ))
;(gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw yLabelXPosition yLabelYPosition yLabel 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
    (set! PI_2 (/ 3.14159 2 ))


    (set! yLabelObj (car (gimp-text-fontname frameIm frameDw yLabelXPosition yLabelYPosition yLabel 0 TRUE fpx PIXELS "Nimbus Mono L Bold Oblique")))
    (set! textWidth  (car (gimp-drawable-width yLabelObj )))
    (set! textHeight (car (gimp-drawable-height yLabelObj )))


    ; 
    ; Draw the axes lines
    ;
    ; 1.Change paint brush to small 

    

    (set! curLayer  )
    (set! points (cons-array 4 'double))
    (aset points 0 xInitial )
    (aset points 1 (- imhc bm ))
    (aset points 2 (- imwc (+ lm )))
    (aset points 3 (- imhc bm ))
    (gimp-palette-set-foreground '(0 0 0 ))
    ;(gimp-paintbrush-default 52 4 points )
;    (gimp-paintbrush-default frameDw  4 points )
    (gimp-pencil frameDw 4 points )
    
    (aset points 0 xInitial )
    (aset points 1 yInitial )
    (aset points 2 xInitial )
    (aset points 3 tm )
    (gimp-paintbrush-default frameDw  4 points )


    ;
    ; Need to position the text to a better location
    ;
    (gimp-drawable-transform-rotate-default yLabelObj (* -1 PI_2 )  1 ( / textWidth 2 ) (/ textHeight 2 ) 0 0)

    (gimp-floating-sel-anchor yLabelObj )


    ;
    ; Title
    ;
;    (set! xTitle (/  imwc 2 ))
;    (set! yTitle tm )
    (set! xTitle (- imcx (* (/ (string-length title) 2.0) fpx)))
;    (set! yTitle (- (/ tm 2.0) (/ fpx 2.0))  )
    (set! ytitle 0 )
    (set! fpTitle  (* 2 fpx ) )
;    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw  xTitle  yTitle title 0 TRUE fpTitle PIXELS "Nimbus Mono L Bold Oblique")))
    (gimp-message (string-append "Adding title at: x=" 
                                 (number->string xTitle ) 
                                 " y=" 
                                 (number->string yTitle )
                                 " and font= " 
                                 (number->string fpx ))
    );message
    
    (gimp-floating-sel-anchor (car (gimp-text-fontname frameIm frameDw xTitle yTitle title 0 TRUE fpTitle PIXELS "Nimbus Mono L Bold Oblique")))
           

;    (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw xTitle yTitle title 0 TRUE 100 PIXELS "Nimbus Mono L Bold Oblique")))

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



(sciber-axes  "/home/jdamon/TestCase1.png" "/home/jdamon/foo2.png" "2" "10" "0" "10" 5 15  "xlabel" "ylabel" "NEW TITLE" 12 13  0.23 "San" )