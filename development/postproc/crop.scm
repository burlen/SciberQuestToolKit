;
;   ____    _ __           ____               __    ____
;  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
; _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
;/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
;
;Copyright 2008 SciberQuest Inc.
;
; auto-crops crops the inFile stores to outFile
(define (sciber-crop inFile outFile )
  (let* ((image (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile )))
         (drawable (car (gimp-image-get-active-layer image)))
         )
    (plug-in-autocrop 0 image drawable)
    (gimp-displays-flush)
    (gimp-file-save RUN-NONINTERACTIVE image drawable outFile outFile )
  );let
);define

(script-fu-register "sciber-crop" 
                    "Crop"
                    "Auto-Crops an image "
                    "Jimi Damon <jdamon@gmail.com>"
                    "Jimi Damon"
                    "2010-06-08"
                    ""
                    SF-STRING "InputFile" "0"
                    SF-STRING "Output File" "0"
                    )
(script-fu-menu-register "sciber-crop" "<Toolbox>/Xtns/Script-Fu/SciberQuest" )

