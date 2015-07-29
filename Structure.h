enum{
  RRQ=1, WRQ, DATA, ACK, ERR
}Opcode;

/*
Error Codes

  Value		Meaning
  0		Not defined, see error message (if any).
  1		File not found.
  2		Access violation.
  3		Disk full or allocation exceeded.
  4		Illegal TFTP operation.
  5		Unknown transfer ID.
  6		File already exists.
  7		No such user.
*/

/*struct RRQ_Header{
  uint16_t ocode;
  char filename[];
  char mode[];
};*/

struct DATA_Header{
  uint16_t ocode;
  uint16_t BlockNumber;
};

struct ACK_Header{
  uint16_t ocode;
  uint16_t BlockNumber;
};

struct ERROR_Header{
  uint16_t ocode;
  uint16_t Errorcode;
  char ErrMsg[];
};
