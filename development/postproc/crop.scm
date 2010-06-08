;
; auto-crops crops the inFile stores to outFile
(define (sciber-crop inFile outFile )
  (let* ((image (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile )))
         (drawable (car (gimp-image-get-active-layer image)))
         )
    (plug-in-autocrop 0 image drawable)
    (gimp-displays-flush)
    (gimp-file-save RUN-NONINTERACTIVE image drawable outFile outFile )
    (display (string-append inFile "->" outFile))
  );let
);define
