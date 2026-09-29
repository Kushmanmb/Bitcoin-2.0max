static EC_KEY* recoverPublicKey(const BIGNUM* r,
                                const BIGNUM* s,
                                int recId,
                                const uint8_t* hash32,
                                bool compressed) {
    if (!r || !s || !hash32 || recId < 0 || recId > 3) {
        return nullptr;
    }

    EC_KEY* key = nullptr;
    EC_GROUP* group = nullptr;
    BN_CTX* ctx = nullptr;
    EC_POINT* R = nullptr;
    EC_POINT* Q = nullptr;

    BIGNUM* order = nullptr;
    BIGNUM* field = nullptr;
    BIGNUM* x = nullptr;
    BIGNUM* z = nullptr;
    BIGNUM* rInv = nullptr;
    BIGNUM* u1 = nullptr;
    BIGNUM* u2 = nullptr;

    group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    ctx = BN_CTX_new();

    if (!group || !ctx) {
        goto cleanup;
    }

    order = BN_new();
    field = BN_new();
    x = BN_dup(r);
    z = BN_bin2bn(hash32, 32, nullptr);
    rInv = BN_new();
    u1 = BN_new();
    u2 = BN_new();

    if (!order || !field || !x || !z || !rInv || !u1 || !u2) {
        goto cleanup;
    }

    if (EC_GROUP_get_order(group, order, ctx) != 1) {
        goto cleanup;
    }

    if (EC_GROUP_get_curve(group, field, nullptr, nullptr, ctx) != 1) {
        goto cleanup;
    }

    // x = r + j*n, where j is the high recovery-id bit.
    if ((recId >> 1) != 0) {
        if (BN_add(x, x, order) != 1) {
            goto cleanup;
        }
    }

    if (BN_cmp(x, field) >= 0) {
        goto cleanup;
    }

    R = EC_POINT_new(group);
    Q = EC_POINT_new(group);

    if (!R || !Q) {
        goto cleanup;
    }

    if (EC_POINT_set_compressed_coordinates(
            group, R, x, recId & 1, ctx) != 1) {
        goto cleanup;
    }

    if (EC_POINT_is_on_curve(group, R, ctx) != 1) {
        goto cleanup;
    }

    // r^-1 mod n
    if (BN_mod_inverse(rInv, r, order, ctx) == nullptr) {
        goto cleanup;
    }

    // u1 = (-z * r^-1) mod n
    if (BN_mod(z, z, order, ctx) != 1) {
        goto cleanup;
    }

    if (!BN_is_zero(z)) {
        if (BN_sub(z, order, z) != 1) {
            goto cleanup;
        }
    }

    if (BN_mod_mul(u1, z, rInv, order, ctx) != 1) {
        goto cleanup;
    }

    // u2 = (s * r^-1) mod n
    if (BN_mod_mul(u2, s, rInv, order, ctx) != 1) {
        goto cleanup;
    }

    // Q = u1*G + u2*R
    if (EC_POINT_mul(group, Q, u1, R, u2, ctx) != 1) {
        goto cleanup;
    }

    if (EC_POINT_is_at_infinity(group, Q) == 1) {
        goto cleanup;
    }

    key = EC_KEY_new();
    if (!key) {
        goto cleanup;
    }

    // Check both OpenSSL operations before returning the key.
    if (EC_KEY_set_group(key, group) != 1 ||
        EC_KEY_set_public_key(key, Q) != 1) {
        EC_KEY_free(key);
        key = nullptr;
        goto cleanup;
    }

    EC_KEY_set_conv_form(
        key,
        compressed
            ? POINT_CONVERSION_COMPRESSED
            : POINT_CONVERSION_UNCOMPRESSED);

cleanup:
    BN_free(order);
    BN_free(field);
    BN_free(x);
    BN_free(z);
    BN_free(rInv);
    BN_free(u1);
    BN_free(u2);

    EC_POINT_free(R);
    EC_POINT_free(Q);
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    return key;
}
