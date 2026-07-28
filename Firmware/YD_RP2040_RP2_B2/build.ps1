# build.ps1 - Build va tu dong nap firmware, khong can giu BOOTSEL thu cong.
# Yeu cau: board da cam san qua USB va dang chay firmware (bat ky firmware nao,
# kha day USB CDC binh thuong). Neu board dang "treo"/khong con nhan dien qua
# USB, xem huong dan Cach 2 (uf2-watch.ps1) o duoi.

$cmake    = "$env:USERPROFILE\.pico-sdk\cmake\v3.31.5\bin\cmake.exe"
$picotool = "$env:USERPROFILE\.pico-sdk\picotool\2.2.0-a4\picotool\picotool.exe"  # sua lai neu path khac

Write-Host "==> Dang build..." -ForegroundColor Cyan
& $cmake --build build

if ($LASTEXITCODE -ne 0) {
    Write-Host "==> Build LOI - xem log o tren, chua nap." -ForegroundColor Red
    exit 1
}

Write-Host "==> Build OK. Dang nap firmware qua picotool (tu dong reboot vao BOOTSEL)..." -ForegroundColor Cyan

# -f : force reboot board vao che do BOOTSEL truoc khi nap (khong can giu nut)
# -x : tu chay chuong trinh ngay sau khi nap xong
& $picotool load .\build\YD_RP2040_RP2_B2.elf -f -x

if ($LASTEXITCODE -eq 0) {
    Write-Host "==> Nap thanh cong, firmware dang chay!" -ForegroundColor Green
} else {
    Write-Host "==> Nap LOI. Neu bao khong tim thay thiet bi, thu Cach 2 (giu BOOTSEL thu cong)." -ForegroundColor Red
}