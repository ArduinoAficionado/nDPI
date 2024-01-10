#include "ndpi_api.h"
#include "ndpi_private.h"
#include "ndpi_classify.h"
#include "fuzz_common_code.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "fuzzer/FuzzedDataProvider.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fuzzed_data(data, size);
  struct ndpi_detection_module_struct *ndpi_info_mod;
  struct ndpi_flow_struct flow;
  u_int8_t protocol_was_guessed;
  u_int32_t i;
  u_int16_t bool_value;
  NDPI_PROTOCOL_BITMASK enabled_bitmask;
  struct ndpi_lru_cache_stats lru_stats;
  struct ndpi_patricia_tree_stats patricia_stats;
  struct ndpi_automa_stats automa_stats;
  int cat;
  u_int16_t pid;
  char *protoname;
  char catname[] = "name";
  struct ndpi_flow_input_info input_info;
  ndpi_proto p, p2;
  char out[128];
  char log_ts[32];
  int value;
  char cfg_value[32];
  char cfg_proto[32];


  /* Just to be sure to have some data */
  if(fuzzed_data.remaining_bytes() < NDPI_MAX_SUPPORTED_PROTOCOLS * 2 + 200)
    return -1;

  /* To allow memory allocation failures */
  fuzz_set_alloc_callbacks_and_seed(size);

  ndpi_info_mod = ndpi_init_detection_module();

  set_ndpi_debug_function(ndpi_info_mod, NULL);

  NDPI_BITMASK_RESET(enabled_bitmask);
  for(i = 0; i < NDPI_MAX_SUPPORTED_PROTOCOLS + NDPI_MAX_NUM_CUSTOM_PROTOCOLS ; i++) {
    if(fuzzed_data.ConsumeBool())
      NDPI_BITMASK_ADD(enabled_bitmask, i);
  }
  if(ndpi_set_protocol_detection_bitmask2(ndpi_info_mod, &enabled_bitmask) == -1) {
    ndpi_exit_detection_module(ndpi_info_mod);
    ndpi_info_mod = NULL;
  }

  ndpi_set_user_data(ndpi_info_mod, (void *)0xabcdabcd); /* Random pointer */
  ndpi_set_user_data(ndpi_info_mod, (void *)0xabcdabcd); /* Twice to trigger overwriting */
  ndpi_get_user_data(ndpi_info_mod);

  /* ndpi_set_config: try to keep the soame order of the definitions in ndpi_main.c.
     + 1 to trigger unvalid parameter error */

  if(fuzzed_data.ConsumeBool())
    ndpi_load_protocols_file(ndpi_info_mod, "protos.txt");
  if(fuzzed_data.ConsumeBool())
    ndpi_load_categories_file(ndpi_info_mod, "categories.txt", NULL);
  if(fuzzed_data.ConsumeBool())
    ndpi_load_risk_domain_file(ndpi_info_mod, "risky_domains.txt");
  if(fuzzed_data.ConsumeBool())
    ndpi_load_malicious_ja3_file(ndpi_info_mod, "ja3_fingerprints.csv");
  if(fuzzed_data.ConsumeBool())
    ndpi_load_malicious_sha1_file(ndpi_info_mod, "sha1_fingerprints.csv");
  /* Note that this function is not used by ndpiReader */
  if(fuzzed_data.ConsumeBool())
    ndpi_load_ipv4_ptree(ndpi_info_mod, "ipv4_addresses.txt", NDPI_PROTOCOL_TLS);

  /* TODO: stub for geo stuff */
  ndpi_load_geoip(ndpi_info_mod, NULL, NULL);

  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 365 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "tls", "certificate_expiration_threshold", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "tls", "application_blocks_tracking.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "tls", "metadata.sha1_fingerprint.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "smtp", "tls_dissection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "imap", "tls_dissection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "pop", "tls_dissection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "ftp", "tls_dissection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "stun", "tls_dissection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "http", "process_response.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 0x01 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "ookla", "aggressiveness", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, "any", "ip_list.load", cfg_value);
  }
  for(i = 0; i < NDPI_MAX_SUPPORTED_PROTOCOLS; i++) {
    if(fuzzed_data.ConsumeBool()) {
      value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
      sprintf(cfg_value, "%d", value);
      sprintf(cfg_proto, "%d", i);
      ndpi_set_config(ndpi_info_mod, cfg_proto, "ip_list.load", cfg_value);
    }
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 255 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "packets_limit_per_flow", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "flow.direction_detection.enable", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "flow_risk_lists.load", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "flow_risk.anonymous_subscriber.list.icloudprivaterelay.load", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "flow_risk.anonymous_subscriber.list.protonvpn.load", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 1 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "flow_risk.crawler_bot.list.load", cfg_value);
  }

  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.ookla.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.ookla.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.bittorrent.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.bittorrent.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.zoom.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.zoom.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.stun.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.stun.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.tls_cert.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.tls_cert.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.mining.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.mining.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.msteams.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.msteams.ttl", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 / 2); /* max / 2 instead of max + 1 to avoid oom on oss-fuzzer */
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.stun_zoom.size", cfg_value);
  }
  if(fuzzed_data.ConsumeBool()) {
    value = fuzzed_data.ConsumeIntegralInRange(0, 16777215 + 1);
    sprintf(cfg_value, "%d", value);
    ndpi_set_config(ndpi_info_mod, NULL, "lru.stun_zoom.ttl", cfg_value);
  }

  ndpi_finalize_initialization(ndpi_info_mod);

  /* Random protocol configuration */
  pid = fuzzed_data.ConsumeIntegralInRange<u_int16_t>(0, NDPI_MAX_SUPPORTED_PROTOCOLS + NDPI_MAX_NUM_CUSTOM_PROTOCOLS + 1); /* + 1 to trigger invalid pid */
  protoname = ndpi_get_proto_by_id(ndpi_info_mod, pid);
  if (protoname) {
    assert(ndpi_get_proto_by_name(ndpi_info_mod, protoname) == pid);
  }
  ndpi_map_user_proto_id_to_ndpi_id(ndpi_info_mod, pid);
  ndpi_map_ndpi_id_to_user_proto_id(ndpi_info_mod, pid);
  ndpi_set_proto_breed(ndpi_info_mod, pid, NDPI_PROTOCOL_SAFE);
  ndpi_set_proto_category(ndpi_info_mod, pid, NDPI_PROTOCOL_CATEGORY_MEDIA);
  ndpi_is_subprotocol_informative(ndpi_info_mod, pid);
  ndpi_get_proto_breed(ndpi_info_mod, pid);

  ndpi_get_proto_by_name(ndpi_info_mod, NULL); /* Error */
  ndpi_get_proto_by_name(ndpi_info_mod, "foo"); /* Invalid protocol */

  /* Custom category configuration */
  cat = fuzzed_data.ConsumeIntegralInRange(static_cast<int>(NDPI_PROTOCOL_CATEGORY_CUSTOM_1),
                                           static_cast<int>(NDPI_PROTOCOL_NUM_CATEGORIES + 1)); /* + 1 to trigger invalid cat */
  ndpi_category_set_name(ndpi_info_mod, static_cast<ndpi_protocol_category_t>(cat), catname);
  ndpi_is_custom_category(static_cast<ndpi_protocol_category_t>(cat));
  ndpi_category_get_name(ndpi_info_mod, static_cast<ndpi_protocol_category_t>(cat));
  ndpi_get_category_id(ndpi_info_mod, catname);

  ndpi_tunnel2str(static_cast<ndpi_packet_tunnel>(fuzzed_data.ConsumeIntegralInRange(static_cast<int>(ndpi_no_tunnel),
                                                                                     static_cast<int>(ndpi_gre_tunnel + 1)))); /* + 1 to trigger invalid value */

  ndpi_get_num_supported_protocols(ndpi_info_mod);
  ndpi_get_proto_defaults(ndpi_info_mod);
  ndpi_get_ndpi_num_custom_protocols(ndpi_info_mod);
  ndpi_get_ndpi_num_supported_protocols(ndpi_info_mod);

  ndpi_self_check_host_match(stdout);

  ndpi_dump_protocols(ndpi_info_mod, stdout);
  ndpi_generate_options(fuzzed_data.ConsumeIntegralInRange(0, 4), stdout);
  ndpi_dump_risks_score(stdout);

  /* Basic code to try testing this "config" */
  bool_value = fuzzed_data.ConsumeBool();
  input_info.in_pkt_dir = fuzzed_data.ConsumeIntegralInRange(0,2);
  input_info.seen_flow_beginning = !!fuzzed_data.ConsumeBool();
  memset(&flow, 0, sizeof(flow));
  std::vector<uint8_t>pkt = fuzzed_data.ConsumeRemainingBytes<uint8_t>();

  ndpi_detection_process_packet(ndpi_info_mod, &flow, pkt.data(), pkt.size(), 0, &input_info);
  p = ndpi_detection_giveup(ndpi_info_mod, &flow, 1, &protocol_was_guessed);

  assert(p.master_protocol == ndpi_get_flow_masterprotocol(ndpi_info_mod, &flow));
  assert(p.app_protocol == ndpi_get_flow_appprotocol(ndpi_info_mod, &flow));
  assert(p.category == ndpi_get_flow_category(ndpi_info_mod, &flow));
  ndpi_get_lower_proto(p);
  ndpi_get_upper_proto(p);
  ndpi_get_flow_error_code(&flow);
  ndpi_get_flow_risk_info(&flow, out, sizeof(out), 1);
  ndpi_get_flow_ndpi_proto(ndpi_info_mod, &flow, &p2);
  ndpi_is_proto(p, NDPI_PROTOCOL_TLS);
  ndpi_http_method2str(flow.http.method);
  ndpi_get_l4_proto_name(ndpi_get_l4_proto_info(ndpi_info_mod, p.app_protocol));
  ndpi_is_subprotocol_informative(ndpi_info_mod, p.app_protocol);
  ndpi_get_http_method(ndpi_info_mod, bool_value ? &flow : NULL);
  ndpi_get_http_url(ndpi_info_mod, &flow);
  ndpi_get_http_content_type(ndpi_info_mod, &flow);
  check_for_email_address(ndpi_info_mod, 0);
  ndpi_get_flow_name(bool_value ? &flow : NULL);
  /* ndpi_guess_undetected_protocol() is a "strange" function. Try fuzzing it, here */
  if(!ndpi_is_protocol_detected(ndpi_info_mod, p)) {
    ndpi_guess_undetected_protocol(ndpi_info_mod, bool_value ? &flow : NULL,
                                   flow.l4_proto);
    if(!flow.is_ipv6) {
      /* Another "strange" function (ipv4 only): fuzz it here, for lack of a better alternative */
      ndpi_find_ipv4_category_userdata(ndpi_info_mod, flow.c_address.v4);

      ndpi_search_tcp_or_udp_raw(ndpi_info_mod, NULL, 0, ntohl(flow.c_address.v4), ntohl(flow.s_address.v4));

      ndpi_guess_undetected_protocol_v4(ndpi_info_mod, bool_value ? &flow : NULL,
                                        flow.l4_proto,
                                        flow.c_address.v4, flow.c_port,
                                        flow.s_address.v4, flow.s_port);
    } else {
      ndpi_find_ipv6_category_userdata(ndpi_info_mod, (struct in6_addr *)flow.c_address.v6);
    }
    /* Another "strange" function: fuzz it here, for lack of a better alternative */
    ndpi_search_tcp_or_udp(ndpi_info_mod, &flow);
  }
  if(!flow.is_ipv6) {
    ndpi_network_ptree_match(ndpi_info_mod, (struct in_addr *)&flow.c_address.v4);

    ndpi_risk_params params[] = { { NDPI_PARAM_HOSTNAME, flow.host_server_name},
                                  { NDPI_PARAM_ISSUER_DN, flow.host_server_name},
                                  { NDPI_PARAM_HOST_IPV4, &flow.c_address.v4} };
    ndpi_check_flow_risk_exceptions(ndpi_info_mod, 3, params);
  }
  /* TODO: stub for geo stuff */
  ndpi_get_geoip_asn(ndpi_info_mod, NULL, NULL);
  ndpi_get_geoip_country_continent(ndpi_info_mod, NULL, NULL, 0, NULL, 0);

  ndpi_free_flow_data(&flow);

  /* Get some final stats */
  for(i = 0; i < NDPI_LRUCACHE_MAX + 1; i++) /* + 1 to test invalid type */
    ndpi_get_lru_cache_stats(ndpi_info_mod, static_cast<lru_cache_type>(i), &lru_stats);
  for(i = 0; i < NDPI_PTREE_MAX + 1; i++) /* + 1 to test invalid type */
    ndpi_get_patricia_stats(ndpi_info_mod, static_cast<ptree_type>(i), &patricia_stats);
  for(i = 0; i < NDPI_AUTOMA_MAX + 1; i++) /* + 1 to test invalid type */
    ndpi_get_automa_stats(ndpi_info_mod, static_cast<automa_type>(i), &automa_stats);


  ndpi_revision();
  ndpi_get_api_version();
  ndpi_get_gcrypt_version();

  ndpi_get_ndpi_detection_module_size();
  ndpi_detection_get_sizeof_ndpi_flow_struct();
  ndpi_detection_get_sizeof_ndpi_flow_tcp_struct();
  ndpi_detection_get_sizeof_ndpi_flow_udp_struct();

  ndpi_get_tot_allocated_memory();
  ndpi_log_timestamp(log_ts, sizeof(log_ts));

  ndpi_free_geoip(ndpi_info_mod);

  ndpi_exit_detection_module(ndpi_info_mod);

  return 0;
}
