import React, { useEffect, useRef } from "react";
import * as THREE from "three";

export function Attitude3D({ pitch, roll, heading }) {
    const mountRef = useRef(null);

    useEffect(() => {
        const width = 280, height = 230;
        const scene = new THREE.Scene();

        const camera = new THREE.PerspectiveCamera(36, width / height, 0.1, 100);
        camera.position.set(2.6, 1.7, 4.6);
        camera.lookAt(0, 0.1, 0);

        const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
        renderer.setSize(width, height);
        renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        if (mountRef.current) {
            mountRef.current.innerHTML = "";
            mountRef.current.appendChild(renderer.domElement);
        }

        scene.add(new THREE.AmbientLight(0xffffff, 0.75));
        const dir = new THREE.DirectionalLight(0xffffff, 0.9);
        dir.position.set(3, 5, 4);
        scene.add(dir);
        const dir2 = new THREE.DirectionalLight(0x0e7c86, 0.25);
        dir2.position.set(-4, 2, -3);
        scene.add(dir2);

        // ground / horizon reference
        const groundGeo = new THREE.CircleGeometry(3.6, 48);
        const groundMat = new THREE.MeshStandardMaterial({ color: 0x0e7c86, transparent: true, opacity: 0.12, side: THREE.DoubleSide });
        const ground = new THREE.Mesh(groundGeo, groundMat);
        ground.rotation.x = -Math.PI / 2;
        ground.position.y = -0.95;
        scene.add(ground);

        const grid = new THREE.GridHelper(7, 14, 0x0e7c86, 0xcbd1da);
        grid.position.y = -0.95;
        grid.material.transparent = true;
        grid.material.opacity = 0.4;
        scene.add(grid);

        // aircraft group
        const plane = new THREE.Group();
        const bodyMat = new THREE.MeshStandardMaterial({ color: 0xebf0f5, metalness: 0.15, roughness: 0.55 });
        const accentMat = new THREE.MeshStandardMaterial({ color: 0x0e7c86, metalness: 0.2, roughness: 0.4 });
        const darkMat = new THREE.MeshStandardMaterial({ color: 0x1b3a5c, metalness: 0.2, roughness: 0.45 });

        const fuselage = new THREE.Mesh(new THREE.CylinderGeometry(0.14, 0.05, 2.1, 16), bodyMat);
        fuselage.rotation.x = Math.PI / 2;
        plane.add(fuselage);

        const nose = new THREE.Mesh(new THREE.ConeGeometry(0.14, 0.36, 16), accentMat);
        nose.rotation.x = Math.PI / 2;
        nose.position.z = -1.23;
        plane.add(nose);

        const wings = new THREE.Mesh(new THREE.BoxGeometry(2.5, 0.05, 0.46), darkMat);
        wings.position.set(0, -0.02, 0.1);
        plane.add(wings);

        const tailH = new THREE.Mesh(new THREE.BoxGeometry(0.95, 0.04, 0.28), darkMat);
        tailH.position.set(0, -0.02, 0.95);
        plane.add(tailH);

        const tailV = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.46, 0.32), accentMat);
        tailV.position.set(0, 0.24, 0.95);
        plane.add(tailV);

        scene.add(plane);

        let raf;
        let t = 0;
        const animate = () => {
            t += 0.012;
            const bob = Math.sin(t) * 0.015;
            plane.rotation.z = -THREE.MathUtils.degToRad(roll);
            plane.rotation.x = THREE.MathUtils.degToRad(pitch) + bob;
            plane.rotation.y = THREE.MathUtils.degToRad(heading);
            renderer.render(scene, camera);
            raf = requestAnimationFrame(animate);
        };
        animate();

        return () => {
            cancelAnimationFrame(raf);
            renderer.dispose();
            groundGeo.dispose();
            groundMat.dispose();
            if (mountRef.current) mountRef.current.innerHTML = "";
        };
    }, [pitch, roll, heading]);

    return <div ref={mountRef} style={{ width: 280, height: 230 }} />;
}
