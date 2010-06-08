;
;
(define (sciber-annotate inFile outFile label fontsize x y )
  (let* ((image (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile )))
         (drawable (car (gimp-image-get-active-layer image)))
         text-float
         )

    (set! text-float (car (gimp-text-fontname image drawable
                                              x y label 0 1 fontsize 0
                                              "Sans")))
    (gimp-floating-sel-anchor text-float)
    (gimp-displays-flush)
    (gimp-file-save RUN-NONINTERACTIVE image drawable outFile outFile )
  );let
);define
