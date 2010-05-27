
(define 
  (sub3-run2-eq inFile outFile label)
    (let* 
      (
      ;
      (widFin 1807)
      (htFin 662)

      ; crop box (wid ht ofs_x ofs_y)
      (cropBox (vector widFin htFin 30 33))
      (reszBox (vector 1857 742 50 40))  
      ;(reszBox (vector (+ 50 widFin) (+ 80 htFin) 50 40))  
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
      ; cover termination surfaces
      (gimp-rect-select frameIm 740 520 418 142 CHANNEL-OP-REPLACE 0 0)
      (gimp-context-set-background '(  0   0 189))
      (gimp-edit-fill frameDw BACKGROUND-FILL)
      (gimp-selection-none frameIm)
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
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 922 707 label 0 TRUE 30 PIXELS "Nimbus Mono L Bold Oblique")))

      ; AXIS-Ticks
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1    40 " 34" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1   678 " 49" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 51   14 "235" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))
      (gimp-floating-sel-to-layer (car (gimp-text-fontname frameIm frameDw 1808 14 "276" 0 TRUE 25 PIXELS "Nimbus Mono L Bold Oblique")))

      ; AXIS LABELS
      (let*
        (
        (xArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/vx-arrow.png")
        (zArrFn "/home/burlen/ext/vis/AGU-09/poster/gimp/hz-arrow.png")
        (xArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm xArrFn))) 
        (zArrL (car (gimp-file-load-layer RUN-NONINTERACTIVE frameIm zArrFn)))
        )
        ; position labels
        (gimp-image-add-layer frameIm xArrL -1) 
        (gimp-layer-set-offsets xArrL 15 313)
        (gimp-image-add-layer frameIm zArrL -1)
        (gimp-layer-set-offsets zArrL 938 10)
      )

      ; SAVE
      (gimp-image-merge-visible-layers frameIm  EXPAND-AS-NECESSARY)
      (set! frameDw (car (gimp-image-get-active-layer frameIm)))
      (gimp-file-save RUN-NONINTERACTIVE frameIm frameDw outFile outFile)
      (gimp-image-delete frameIm)

      (display (string-append inFile "->" outFile))
   )
)

