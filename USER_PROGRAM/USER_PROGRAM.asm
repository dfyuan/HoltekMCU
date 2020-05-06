        

message '***************************************************************'
message '*PROJECT NAME :USER PROGRAM CODE                              *'
message '*     VERSION :                                               *'
message '*     IC BODY :                                               *'
message '* ICE VERSION :                                               *'
message '*      REMARK :                                               *'
message '***************************************************************'

                INCLUDE USER_PROGRAM.INC

                PUBLIC  _USER_PROGRAM_INITIAL
                PUBLIC  _USER_PROGRAM 


                ;========================
                ;=USER DATA DEFINE      =
                ;========================
USER_DATA       .SECTION   'DATA'



                ;========================
                ;=USER PROGRAM          =
                ;========================
USER_PROGRAM    .SECTION   'CODE'
                           
;;***********************************************************                           
;;*SUB. NAME:USER INITIAL PROGRAM                           *                           
;;*INPUT    :                                               *                           
;;*OUTPUT   :                                               *                           
;;*USED REG.:                                               *                           
;;*FUNCTION :                                               *                           
;;***********************************************************                           
                      ;;************************
_USER_PROGRAM_INITIAL:;;* USER_PROGRAM_INITIAL *
                      ;;************************


                CLR     PAC
                CLR     PBC
                CLR     PCC
                CLR     PDC
                CLR     PEC
                ;;CLR     PFC

                CLR     PA 
                CLR     PB 
                CLR     PC 
                CLR     PD 
                CLR     PE 
                ;;CLR     PF 



                RET




;;***********************************************************                           
;;*SUB. NAME:USER_MAIN                                      *                           
;;*INPUT    :                                               *                           
;;*OUTPUT   :                                               *                           
;;*USED REG.:                                               *                           
;;*FUNCTION :                                               *                           
;;***********************************************************                           
                ;;********************
_USER_PROGRAM:  ;;USER PROGRAM ENTRY *
                ;;********************

                SNZ     _SCAN_CYCLEF
                RET


                ;-TEST
                LCLR     PAS0.2
                LCLR     PAS0.3
                CLR     PAC.1
              
                SNZ     _ANY_KEY_PRESSF
                CLR     PA.1
                SZ      _ANY_KEY_PRESSF
                SET     PA.1
             ;
             ;  CALL    _GET_KEY_BITMAP
             ;  SZ      _DATA_BUF[0].0
             ;  SET     PA.1
             ;  SNZ     _DATA_BUF[0].0
             ;  CLR     PA.1

              ; ;-TEST
              ; CLR     PAC.3
              ; MOV     A,00001000B
              ; SZ      _SCAN_CYCLEF
              ; XORM    A,PA

                RET


                


