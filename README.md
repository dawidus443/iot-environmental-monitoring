# IoT Environmental Monitoring — Data Engineering Portfolio

Full-stack data engineering project demonstrating **pipeline architecture** from hardware collection through cloud storage, transformations, orchestration, and analytics.

**Stack:** ESP32 (C++) → Supabase (PostgreSQL) → dbt → Dagster → Grafana

## 📁 Modules

- **[firmware/](firmware/)** — ESP32 sensor data collection (Phase 1 ✅)
- **[backend/](backend/)** — dbt transformations (Phase 2)
- **[orchestration/](orchestration/)** — Dagster workflows (Phase 3)
- **[grafana/](grafana/)** — Visualization dashboards
- **[docs/](docs/)** — Architecture and design

## 🚀 Quick Start

See [firmware/README.md](firmware/README.md) for ESP32 setup instructions.

## 🛣️ Roadmap

- **Phase 1** ✅ — Hardware + Supabase ingestion (current)
- **Phase 2** — dbt data models & transformations
- **Phase 3** — Dagster asset orchestration
- **Phase 4** — ML anomaly detection + Kafka streaming

---

**Status:** Active Development | **Last Updated:** March 2026
