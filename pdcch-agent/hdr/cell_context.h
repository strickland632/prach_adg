struct cell_context {
    srsran_mib_nr_t    mib{};
    srsran_plmn_id_t   plmn{};
    uint16_t           pci = 0;
    // Derived from MIB (TS 38.213 §13) – needed for CORESET-0
    uint32_t           ssb_offset_khz = 0;
    uint16_t           rb_offset  = 0;
    uint16_t           rb_length  = 0;
  };
  