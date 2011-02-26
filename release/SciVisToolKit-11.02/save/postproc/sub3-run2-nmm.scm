
(define 
  (sub3-run2-nmm inFile outFile label)
    (let* 
      (
      ;
      (widFin 1853)
      (htFin 732)

      ; crop box (wid ht ofs_x ofs_y)
      (cropBox (vector widFin htFin 12 84))
      ;(reszBox (vector (+ 50 widFin) (+ 80 htFin) 50 40))  
      (reszBox (vector 1903 812 50 40)) 
      ; load the original
      (frameIm (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (frameDw (car (gimp-image-get-active-layer frameIm)))
      )

      ; crop
      (gimp-image-crop frameIm 
        (vector-ref cropBox 0) 
        (vector-ref cropBox 1)
        (vector-ref cropBox 2)
        (vector-ref cropBox 3)
      )
      ; resize
      (gimp-context-set-background '(  0   0   0))
      (gimp-context-set-foreground '(255 255 255))

      (gimp-image-resize frameIm 
        (vector-ref reszBox 0) 
        (vector-ref reszBox 1)
        (vector-ref reszBox 2)
        (vector-ref reszBox 3)
      )
      (gimp-layer-resize frameDw 
        (vector-ref reszBox 0) 
        (vector-ref reszBox 1)
        (vector-ref reszBox 2)
        (vector-ref reszBox 3)
      )

      ; TITLE
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 903 776 label 0 TRUE 30 PIXELS "Nimbus Mono L Bold Oblique")))

      ; AXIS-Ticks
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1    40 " 33" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1   744 " 45" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 51   14 "249" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1855 14 "265" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))

      ; AXIS LABELS
      (let*
        (
        (xArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/vx-arrow.png")
        (yArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/hy-arrow.png")
        (xArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm xArrFn))) 
        (yArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm yArrFn)))
        )
        ; position labels
        (gimp-image-add-layer frameIm xArrL -1) 
        (gimp-layer-set-offsets xArrL 15 354)
        (gimp-image-add-layer frameIm yArrL -1)
        (gimp-layer-set-offsets yArrL 934 10)
      )

      ; SAVE
      (gimp-image-merge-visible-layers frameIm  EXPAND-AS-NECESSARY)
      (set! frameDw (car (gimp-image-get-active-layer frameIm)))
      (gimp-file-save RUN-NONINTERACTIVE frameIm frameDw outFile outFile)
      (gimp-image-delete frameIm)

      (display (string-append inFile "->" outFile))
   )
)

