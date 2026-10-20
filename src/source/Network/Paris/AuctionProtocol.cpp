#include "stdafx.h"
#include "UI/NewUI/Character/ClassicPanelFrame.h"
#include "UI/Legacy/UIControls.h"
#include "AuctionProtocol.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/ExtendedItemData.h"
#include "../../../ThirdParty/json.hpp"
#include <objbase.h>
#include <cstring>
#include <cstdio>

namespace Network::Paris::Auctions
{
    Snapshot Data;
    DWORD Received = 0;
    namespace
    {
        constexpr int MaximumJsonBytes = 60000, RequestLength = 192;
        std::wstring Wide(const std::string& text)
        {
            if (text.size() > 1024) throw std::runtime_error("Auction text limit");
            const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
            if (!count && !text.empty()) throw std::runtime_error("Auction UTF8");
            std::wstring result(count, L'\0');
            if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), count);
            return result;
        }

        int Nibble(char value)
        {
            if (value >= '0' && value <= '9') return value - '0';
            if (value >= 'A' && value <= 'F') return value - 'A' + 10;
            throw std::runtime_error("Auction item hex");
        }

        std::vector<BYTE> ReadItem(const std::string& text)
        {
            if (text.empty()) return {};
            if (text.size() < 10 || text.size() > 30 || text.size() % 2) throw std::runtime_error("Auction item length");
            std::vector<BYTE> result(text.size() / 2);
            for (size_t index = 0; index < result.size(); ++index) result[index] = static_cast<BYTE>((Nibble(text[index * 2]) << 4) | Nibble(text[index * 2 + 1]));
            if (Network::Items::ExtendedItemLength(result) != result.size()) throw std::runtime_error("Auction item structure");
            return result;
        }

        Row ReadListing(const nlohmann::json& json)
        {
            Row row;
            row.Id = json.at("id"); row.Revision = json.at("revision"); row.Name = Wide(json.at("name")); row.Seller = Wide(json.at("seller"));
            row.ItemData = ReadItem(json.at("data")); row.Code = json.at("code"); row.Level = json.at("level"); row.Currency = json.at("currency");
            row.Price = json.at("price"); row.NextBid = json.at("nextBid"); row.BuyNow = json.at("buyNow"); row.Ends = json.at("ends"); row.Status = json.at("status");
            row.Owned = json.at("owned"); row.CanCancel = json.at("canCancel"); row.Highest = json.at("highest");
            if (row.Id.size() != 36 || row.Revision < 1 || row.Code < 0 || row.Code >= 8192 || row.Level < 0 || row.Level > 255
                || row.Currency < 0 || row.Currency > 7 || row.Price < 1 || row.NextBid < 1 || row.BuyNow < 0 || row.Status < 0 || row.Status > 3)
                throw std::runtime_error("Auction listing range");
            return row;
        }

        Row ReadRow(const nlohmann::json& json, int tab)
        {
            if (tab == 0 || tab == 4) return ReadListing(json);
            Row row;
            if (tab == 1)
            {
                row.Name = Wide(json.at("name")); row.Slot = json.at("slot"); row.Code = json.at("code"); row.Level = json.at("level"); row.ItemData = ReadItem(json.at("data"));
                row.SourceId = json.at("itemId");
                if (row.Slot < 12 || row.Slot >= 204 || row.Code < 0 || row.Code >= 8192 || row.Level < 0 || row.Level > 255 || row.ItemData.empty() || row.SourceId.size() != 36)
                    throw std::runtime_error("Auction inventory range");
                return row;
            }
            row = ReadListing(json.at("listing")); row.Amount = json.at("amount"); row.Time = json.at("time");
            if (tab == 2) row.State = json.at("state");
            else
            {
                row.DeliveryId = json.at("id"); row.ItemDelivery = json.at("item");
                if (!row.ItemDelivery) row.Currency = json.at("currency");
                if (row.DeliveryId.size() != 36 || row.Currency < 0 || row.Currency > 7) throw std::runtime_error("Auction delivery range");
            }
            if (row.Amount < 0) throw std::runtime_error("Auction payment range");
            return row;
        }
    }

    bool Receive(const BYTE* packet, int size)
    {
        if (!packet || size < 6 || size > MaximumJsonBytes + 6 || packet[0] != 0xC2 || packet[3] != 0xFA || packet[4] != 0xD6
            || packet[5] != 1 || ((packet[1] << 8) | packet[2]) != size) return false;
        try
        {
            const auto json = nlohmann::json::parse(packet + 6, packet + size);
            Snapshot next;
            next.Tab = json.at("tab"); next.Page = json.at("page"); next.SubTab = json.at("subTab"); next.Decision = json.at("decision");
            next.Group=json.value("group",255);next.Query=Wide(json.value("query",std::string{}));
            if((next.Group!=255 && (next.Group<0 || next.Group>21)) || next.Query.size()>64)return false;
            next.Action = json.value("action",0);
            if(next.Action<0 || next.Action>5)return false;
            next.Enabled = json.at("enabled"); next.More = json.at("more"); next.ServerTime = json.at("now"); next.Fee = json.at("fee");
            if (next.Tab < 0 || next.Tab > 4 || next.Page < 0 || next.Page > 100 || next.SubTab < 0 || next.SubTab > 1
                || next.Decision < 0 || next.Decision > 12 || next.Fee < 0 || json.at("labels").size() > 80 || json.at("rows").size() > (next.Tab == 1 ? 192 : 7)) return false;
            next.Durations = json.at("durations").get<std::vector<int>>(); next.Currencies = json.at("currencies").get<std::vector<int>>();
            if (next.Durations.empty() || next.Durations.size() > 12 || next.Currencies.empty() || next.Currencies.size() > 8) return false;
            for (int value : next.Durations) if (value < 1 || value > 43200) return false;
            for (int value : next.Currencies) if (value < 0 || value > 7) return false;
            for (auto it = json.at("labels").begin(); it != json.at("labels").end(); ++it)
                if (it.key().size() <= 64) next.Labels[it.key()] = Wide(it.value()); else return false;
            for (const auto& value : json.at("rows")) next.Rows.push_back(ReadRow(value, next.Tab));
            Data = std::move(next); Received = timeGetTime(); return true;
        }
        catch (const std::exception& exception)
        {
            FILE* log=nullptr;fopen_s(&log,"ParisAuctionProtocol.log","a");
            if(log){std::fprintf(log,"Rejected auction snapshot: %s\n",exception.what());std::fclose(log);}
            return false;
        }
    }

    void Clear() { Data = Snapshot{}; Received = 0; }
    const std::wstring& Label(const char* key)
    {
        static const std::map<std::string,std::wstring> initial = {
            {"AuctionTitle",L"Subastas"},{"Publish",L"Publicar"},{"MyOffers",L"Mis ofertas"},{"Deliveries",L"Entregas"},{"MyAuctions",L"Mis subastas"},
            {"Search",L"Buscar"},{"All",L"Todos"},{"Item",L"Item"},{"Name",L"Nombre"},{"Options",L"Opciones"},{"Price",L"Precio"},{"Seller",L"Vendedor"},{"Time",L"Tiempo"},
            {"Buy",L"Comprar"},{"Bid",L"Pujar"},{"Currency",L"Moneda de venta"},{"Duration",L"Duracion"},{"PublicationFeeZen",L"Costo de publicacion"},
            {"MyBids",L"Ofertas realizadas"},{"Purchases",L"Historial de compras"},{"ItemsToReceive",L"Items por recibir"},{"CoinsToReceive",L"Monedas por recibir"},
            {"ActiveListings",L"Publicaciones activas"},{"History",L"Historial"},{"Cancel",L"Cancelar"},{"Collect",L"Recoger"},{"Available",L"Disponible"},
            {"Won",L"Ganaste"},{"Active",L"Activa"},{"Outbid",L"Superada"},{"Expired",L"Expirada"},{"Cancelled",L"Cancelada"},{"Sold",L"Vendida"},
            {"NoListings",L"No hay subastas publicadas."},{"NoOffers",L"No tienes ofertas ni compras en esta sección."},{"NoDeliveries",L"No tienes entregas pendientes en esta sección."},{"NoOwnListings",L"No tienes publicaciones en esta sección."},{"BuyNowShort",L"Compra directa"},
            {"StartingBid",L"Puja inicial"},{"BuyNow",L"Compra inmediata"},{"Confirm",L"Confirmar"},{"ChooseInventory",L"Selecciona un item del inventario"},
            {"Disabled",L"Subastas desactivadas"},{"Pending",L"Procesando..."},{"Loading",L"Conectando con Subastas..."},{"Date",L"Fecha"},{"State",L"Estado"},
            {"Offer",L"Oferta"},{"Durability",L"Durabilidad"},{"ItemDefense",L"Defensa"},{"Strength",L"Fuerza"},{"Agility",L"Agilidad"},{"Skill",L"Skill"},{"Luck",L"Suerte"},
            {"Currency0",L"Zen"},{"Currency1",L"Paris Coins"},{"Currency2",L"WPoints"},{"Currency3",L"Chaos"},{"Currency4",L"Bless"},{"Currency5",L"Soul"},{"Currency6",L"Life"},{"Currency7",L"Creation"}
        };
        static const std::wstring empty;
        auto found = Data.Labels.find(key);
        if(found!=Data.Labels.end()) return found->second;
        auto fallback=initial.find(key);return fallback==initial.end()?empty:fallback->second;
    }

    bool Request(BYTE action, BYTE tab, BYTE page, BYTE subTab, const std::wstring& query, BYTE group,
        const Row* row, int price, int buyNow, int duration, BYTE currency)
    {
        if (!SocketClient || action > 5 || tab > 4 || page > 100 || subTab > 1 || currency > 7) return false;
        BYTE packet[RequestLength] = { 0xC1, RequestLength, 0xFA, 0xD5, 1, action, tab, page, subTab, currency, 0, group };
        std::memcpy(packet + 12, &price, sizeof(price)); std::memcpy(packet + 16, &buyNow, sizeof(buyNow)); std::memcpy(packet + 20, &duration, sizeof(duration));
        const int textSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, query.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (textSize < 1 || textSize > 64) return false;
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, query.c_str(), -1, reinterpret_cast<char*>(packet + 112), 64, nullptr, nullptr);
        if (row)
        {
            const auto& id = action == 5 ? row->DeliveryId : row->Id;
            if (!id.empty() && id.size() != 36 || row->ItemData.size() > 16) return false;
            std::memcpy(packet + 32, id.data(), id.size()); std::memcpy(packet + 24, &row->Revision, sizeof(row->Revision));
            packet[10] = static_cast<BYTE>(row->Slot);
            if (action == 1)
            {
                GUID nativeIdentity{};
                const auto identityText = L"{" + Wide(row->SourceId) + L"}";
                if (FAILED(CLSIDFromString(identityText.c_str(), &nativeIdentity))) return false;
                std::memcpy(packet + 176, &nativeIdentity, sizeof(nativeIdentity));
            }
        }
        if (action != 0)
        {
            GUID identity{}; if (FAILED(CoCreateGuid(&identity))) return false;
            std::snprintf(reinterpret_cast<char*>(packet + 72), 37, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                identity.Data1, identity.Data2, identity.Data3, identity.Data4[0], identity.Data4[1], identity.Data4[2], identity.Data4[3],
                identity.Data4[4], identity.Data4[5], identity.Data4[6], identity.Data4[7]);
        }
        SocketClient->Send(packet, sizeof(packet)); return true;
    }
}
